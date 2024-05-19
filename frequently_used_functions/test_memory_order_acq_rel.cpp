#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>

std::atomic<int> sync(0), datas[5];

void sync_store_test()
{
    for (int i = 0; i < 5; ++i)
    {
        datas[i].store(i + 1, std::memory_order_relaxed);
    }
    sync.store(1, std::memory_order_release);
}

void sync_exchange_test()
{
    int expected = 1;

    while (!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
    {
        expected = 1;
    }
}

void sync_load_test()
{
    while (sync.load(std::memory_order_acquire) < 2)
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
    std::thread t1(sync_store_test);
    std::thread t2(sync_exchange_test);
    std::thread t3(sync_load_test);

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
