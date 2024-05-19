#include <iostream>
#include <thread>
#include <mutex>
#include <string>

class X
{
private:
    std::string s;
    std::recursive_mutex m;

public:
    void func1()
    {
        std::lock_guard lk(m);
        s = "func1";
        std::cout << "In func1, s = " << s << std::endl;
    }

    void func2()
    {
        std::lock_guard lk(m);
        s = "func2";
        std::cout << "In func2, s = " << s << std::endl;
        func1();
        std::cout << "Back in func2, s = " << s << std::endl;
    }
};

int main()
{
    X test;
    std::thread t1(&X::func1, &test);
    std::thread t2(&X::func2, &test);

    t1.join();
    t2.join();

    return 0;
}
