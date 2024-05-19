#include <iostream>
#include <thread>
#include <future>
#include <functional>

int f(int x, int y)
{
    return x * y;
}

void task_lambda()
{
    std::packaged_task<int(int, int)> task([](int a, int b) {
        return a * b;
    });
    std::future<int> res = task.get_future();
    task(6, 6);
    std::cout << "task_lambda: " << res.get() << std::endl;
}

void task_bind()
{
    std::packaged_task<int()> task(std::bind(f, 2, 6));
    std::future<int> res = task.get_future();
    task();
    std::cout << "task_bind: " << res.get() << std::endl;
}

void task_thread()
{
    std::packaged_task<int(int, int)> task(f);
    std::future<int> res = task.get_future();

    std::thread t(std::move(task), 2, 10);
    t.join();
    std::cout << "task_thread: " << res.get() << std::endl;
}

int main()
{
    task_lambda();
    task_bind();
    task_thread();

    return 0;
}
