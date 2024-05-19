#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <iterator>
#include <algorithm>

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

template <typename Iterator, typename Func>
void thread_for_each(Iterator first, Iterator last, Func f)
{
    unsigned long const length = std::distance(first, last);

    if (length == 0)
    {
        return;
    }
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::vector<std::future<void>> fs(num_threads - 1);
    std::vector<std::thread> ts(num_threads - 1);
    join_threads joiners(ts);
    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        std::packaged_task<void(void)> task([=]() { std::for_each(block_start, block_end, f); });
        fs[i] = task.get_future();
        ts[i] = std::thread(std::move(task));
        block_start = block_end;
    }
    std::for_each(block_start, last, f);
    for (auto& t : fs)
    {
        t.get();
    }
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(10000000);
    std::iota(v.begin(), v.end(), 0);
    thread_for_each(v.begin(), v.end(), [](int& x) { ++x; });

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
