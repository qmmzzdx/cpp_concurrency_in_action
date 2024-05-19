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

template <typename Iterator, typename MatchType>
Iterator thread_find(Iterator first, Iterator last, MatchType match)
{
    struct find_element
    {
        void operator()(Iterator begin, Iterator end, MatchType match, std::promise<Iterator>* result, std::atomic<bool>* done_flag)
        {
            try
            {
                while (begin != end && !done_flag->load())
                {
                    if (*begin == match)
                    {
                        result->set_value(begin);
                        done_flag->store(true);
                        return;
                    }
                    ++begin;
                }
            }
            catch (...)
            {
                try
                {
                    result->set_exception(std::current_exception());
                    done_flag->store(true);
                }
                catch (...)
                {
                }
            }
        }
    };
    unsigned long const length = std::distance(first, last);

    if (length == 0)
    {
        return last;
    }
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::promise<Iterator> result;
    std::atomic<bool> done_flag(false);
    std::vector<std::thread> ts(num_threads - 1);
    {
        join_threads joiner(ts);
        Iterator block_start = first;
        for (unsigned long i = 0; i < num_threads - 1; ++i)
        {
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            ts[i] = std::thread(find_element(), block_start, block_end, match, &result, &done_flag);
            block_start = block_end;
        }
        find_element()(block_start, last, match, &result, &done_flag);
    }
    if (!done_flag.load())
    {
        return last;
    }
    return result.get_future().get();
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(10000000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Target: " << *thread_find(v.begin(), v.end(), 1000000) << std::endl;

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
