#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
#include <iterator>
#include <algorithm>

template <typename Iterator, typename Func>
void async_for_each(Iterator first, Iterator last, Func f)
{
    unsigned long const length = std::distance(first, last);

    if (length == 0)
    {
        return;
    }
    unsigned long const min_per_thread = 25;
    if (length >= 2 * min_per_thread)
    {
        Iterator const mid_point = first + length / 2;
        std::future<void> first_half = std::async(&async_for_each<Iterator, Func>, first, mid_point, f);
        async_for_each(mid_point, last, f);
        first_half.get();
        return;
    }
    std::for_each(first, last, f);
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(100000);
    std::iota(v.begin(), v.end(), 0);
    async_for_each(v.begin(), v.end(), [](int& x) { ++x; });

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
