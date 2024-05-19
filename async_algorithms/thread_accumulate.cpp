#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <iterator>

class join_threads
{
private:
    std::vector<std::thread>& ts;

public:
    explicit join_threads(std::vector<std::thread>& _ts) : ts(_ts) {}

    ~join_threads()
    {
        for (auto& t : ts)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }
};

template <typename Iterator, typename T>
struct accumulate_block
{
    T operator()(Iterator first, Iterator last)
    {
        return std::accumulate(first, last, T());
    }
};

template <typename Iterator, typename T>
T thread_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);

    if (length == 0)
    {
        return init;
    }
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::vector<std::future<T>> fs(num_threads - 1);
    std::vector<std::thread> ts(num_threads - 1);
    join_threads joiners(ts);
    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<T(Iterator, Iterator)> task((accumulate_block<Iterator, T>()));
        fs[i] = task.get_future();
        ts[i] = std::thread(std::move(task), block_start, block_end);
        block_start = block_end;
    }
    T last_result = accumulate_block<Iterator, T>()(block_start, last);
    T result = init;
    for (unsigned long i = 0; i < num_threads - 1; ++i)
    {
        result += fs[i].get();
    }
    result += last_result;
    return result;
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(10000000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Sum: " << thread_accumulate(v.begin(), v.end(), 0LL) << std::endl;

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
