#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <numeric>
#include <algorithm>

template <typename Iterator, typename T>
T async_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);
    unsigned long const max_chunk_size = 25;
    if (length > max_chunk_size)
    {
        Iterator mid_point = first;
        std::advance(mid_point, length / 2);
        std::future<T> first_half_result = std::async(async_accumulate<Iterator, T>, first, mid_point, init);
        T second_half_result = async_accumulate(mid_point, last, T());
        return first_half_result.get() + second_half_result;
    }
    return std::accumulate(first, last, init);
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    std::vector<int> v(100000);
    std::iota(v.begin(), v.end(), 1);
    std::cout << "Sum: " << async_accumulate(v.begin(), v.end(), 0LL) << std::endl;

    const auto end = std::chrono::steady_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}
