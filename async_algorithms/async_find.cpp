#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <iterator>

template <typename Iterator, typename MatchType>
Iterator async_find_impl(Iterator first, Iterator last, MatchType match, std::atomic<bool>& done)
{
    try
    {
        unsigned long const length = std::distance(first, last);
        unsigned long const min_per_thread = 25;
        if (length < 2 * min_per_thread)
        {
            while (first != last && !done.load())
            {
                if (*first == match)
                {
                    done = true;
                    return first;
                }
                ++first;
            }
            return last;
        }
        Iterator const mid_point = first + (length / 2);
        std::future<Iterator> async_result = std::async(&async_find_impl<Iterator, MatchType>, mid_point, last, match, std::ref(done));
        Iterator const direct_result = async_find_impl(first, mid_point, match, done);
        return direct_result == mid_point ? async_result.get() : direct_result;
    }
    catch (...)
    {
        done = true;
        throw;
    }
}

template <typename Iterator, typename MatchType>
Iterator async_find(Iterator first, Iterator last, MatchType match)
{
    std::atomic<bool> done(false);
    return async_find_impl(first, last, match, done);
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(100000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Target: " << *async_find(v.begin(), v.end(), 100000) << std::endl;

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
