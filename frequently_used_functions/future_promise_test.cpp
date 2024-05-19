#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <numeric>
using vit = std::vector<int>::iterator;

std::mutex mtx;

void calculate(vit first, vit last, std::promise<int> sum)
{
    sum.set_value(std::accumulate(first, last, 0));
}

void func_shared(std::shared_future<int> sf)
{
    std::shared_future<int> cf = sf;
    auto res = cf.get();
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << std::this_thread::get_id() << " " << res << std::endl;
}

int main()
{
    std::vector<int> nums{1, 2, 3, 4, 5};
    std::promise<int> sum_promise;
    std::future<int> sum_future = sum_promise.get_future();

    std::thread t(calculate, nums.begin(), nums.end(), std::move(sum_promise));
    std::cout << "Sum = " << sum_future.get() << std::endl;
    t.join();

    std::vector<std::thread> threads;
    std::promise<int> test_shared;
    std::shared_future<int> sf = test_shared.get_future();

    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(func_shared, sf);
    }
    test_shared.set_value(666);
    for (auto &th : threads)
    {
        th.join();
    }

    return 0;
}
