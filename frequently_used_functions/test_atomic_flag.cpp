#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

class spinlock_mutex
{
private:
    std::atomic_flag spinlock_mtx;

public:
    spinlock_mutex() : spinlock_mtx(ATOMIC_FLAG_INIT) {}

    void lock()
    {
        while (spinlock_mtx.test_and_set(std::memory_order_acquire))
        {
            continue;
        }
    }

    void unlock()
    {
        spinlock_mtx.clear(std::memory_order_release);
    }
};

spinlock_mutex smtx;

void test_spinlock(int n)
{
    for (int i = 0; i < 2; ++i)
    {
        smtx.lock();
        std::cout << "n = " << n << ", thread_id = ";
        std::cout << std::this_thread::get_id() << std::endl;
        smtx.unlock();
    }
}

int main()
{
    std::vector<std::thread> ts;

    for (int i = 0; i < 10; ++i)
    {
        ts.emplace_back(test_spinlock, i);
    }
    for (auto &t : ts)
    {
        t.join();
    }

    return 0;
}
