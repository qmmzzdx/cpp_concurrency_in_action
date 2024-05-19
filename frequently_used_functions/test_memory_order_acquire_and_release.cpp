#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>

std::atomic<int> datas[5];
std::atomic<bool> sync1(false), sync2(false);

void sync1_store_test()
{
    for (int i = 0; i < 5; ++i)
    {
        datas[i].store(i + 1, std::memory_order_relaxed);
    }
    sync1.store(true, std::memory_order_release);
}

void sync2_store_test()
{
    while (!sync1.load(std::memory_order_acquire))
    {
        continue;
    }
    sync2.store(std::memory_order_release);
}

void sync2_load_test()
{
    while (!sync2.load(std::memory_order_acquire))
    {
        continue;
    }
    for (int i = 0; i < 5; ++i)
    {
        assert(datas[i].load(std::memory_order_relaxed) == i + 1);
    }
}

int main()
{
    std::thread t1(sync1_store_test);
    std::thread t2(sync2_store_test);
    std::thread t3(sync2_load_test);

    t1.join();
    t2.join();
    t3.join();
    for (int i = 0; i < 5; ++i)
    {
        std::cout << "i = " << datas[i].load(std::memory_order_relaxed);
        std::cout << std::endl;
    }

    return 0;
}
