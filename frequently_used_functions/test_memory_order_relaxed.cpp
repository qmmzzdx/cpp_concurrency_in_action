#include <iostream>
#include <thread>
#include <atomic>
constexpr int loop_count = 10;

struct read_values
{
    int x, y, z;
};

std::atomic<int> x(0), y(0), z(0);
std::atomic<bool> go(false);
read_values values1[loop_count];
read_values values2[loop_count];
read_values values3[loop_count];
read_values values4[loop_count];
read_values values5[loop_count];

void increment(std::atomic<int> *var_to_inc, read_values *vals)
{
    while (!go)
    {
        std::this_thread::yield();
        // 建议操作系统令当前线程主动让出CPU时间片, 让其他线程有机会运行. 调用后, 当前线程可能会进入就绪状态, 等待系统重新调度
    }
    for (int i = 0; i < loop_count; ++i)
    {
        vals[i].x = x.load(std::memory_order_relaxed);
        vals[i].y = y.load(std::memory_order_relaxed);
        vals[i].z = z.load(std::memory_order_relaxed);
        var_to_inc->store(i + 1, std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

void read_vals(read_values *vals)
{
    while (!go)
    {
        std::this_thread::yield();
    }
    for (int i = 0; i < loop_count; ++i)
    {
        vals[i].x = x.load(std::memory_order_relaxed);
        vals[i].y = y.load(std::memory_order_relaxed);
        vals[i].z = z.load(std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

void print(read_values *v)
{
    for (int i = 0; i < loop_count; ++i)
    {
        std::cout << "(" << v[i].x << ", " << v[i].y << ", " << v[i].z << ")" << std::endl;
    }
    std::cout << std::endl;
}

int main()
{
    std::thread t1(increment, &x, values1);
    std::thread t2(increment, &y, values2);
    std::thread t3(increment, &z, values3);
    std::thread t4(read_vals, values4);
    std::thread t5(read_vals, values5);

    go = true;
    t5.join();
    t4.join();
    t3.join();
    t2.join();
    t1.join();
    print(values1);
    print(values2);
    print(values3);
    print(values4);
    print(values5);

    return 0;
}
