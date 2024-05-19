#include <iostream>
#include <list>
#include <future>
#include <string>
#include <algorithm>

template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T &pivot = *result.begin();

    auto divide_point = std::partition(input.begin(), input.end(), [&](const T &t) {
        return t < pivot;
    });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);
    return result;
}

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T &pivot = *result.begin();

    auto divide_point = std::partition(input.begin(), input.end(), [&](const T &t) {
        return t < pivot;
    });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    std::future<std::list<T>> new_lower(std::async(&parallel_quick_sort<T>, std::move(lower_part)));
    auto new_higher(parallel_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

int main()
{
    auto test_sequential = sequential_quick_sort(std::list<int>{6, 5, 8, 2, 1});
    auto test_parallel = parallel_quick_sort(std::list<int>{3, 2, 7, 6, 8, 9});

    auto check = [&](std::string s, std::list<int> &lists)
    {
        std::cout << s << std::endl;
        for (auto &t : lists)
        {
            std::cout << t << " ";
        }
        std::cout << std::endl;
    };
    check("Test sequential_quick_sort:", test_sequential);
    check("Test parallel_quick_sort:", test_parallel);

    return 0;
}
