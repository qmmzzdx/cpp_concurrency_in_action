#include <iostream>
#include <thread>

void some_function()
{
    std::cout << "some_function" << std::endl;
}

void some_other_function(int n)
{
    std::cout << "some_other_function " << n << std::endl;
}

std::thread f()
{
    void some_function();
    std::cout << "fid = " << std::this_thread::get_id() << std::endl;
    return std::thread(some_function);
}
std::thread g()
{
    void some_other_function(int);
    std::cout << "gid = " << std::this_thread::get_id() << std::endl;
    std::thread t(some_other_function, 42);
    return t;
}

int main()
{
    std::thread t1 = f();
    t1.join();
    std::thread t2 = g();
    t2.join();
    std::cout << "mainid = " << std::this_thread::get_id() << std::endl;

    return 0;
}
