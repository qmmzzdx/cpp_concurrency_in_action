#include <iostream>
#include <thread>

class scoped_thread
{
private:
    std::thread t;

public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_))
    {
        if (!t.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread()
    {
        t.join();
    }
    scoped_thread(scoped_thread const &) = delete;
    scoped_thread &operator=(scoped_thread const &) = delete;
};

void do_something(int &i)
{
    ++i;
}

struct func
{
    int &i;
    func(int &i_) : i(i_) {}
    void operator()()
    {
        for (unsigned j = 0; j < 1000000; ++j)
        {
            do_something(i);
        }
        std::cout << "i = " << i << std::endl;
    }
};

void do_something_in_current_thread()
{
    std::cout << "do_something_in_current_thread" << std::endl;
}

void f()
{
    int some_local_state = 0;
    scoped_thread t{std::thread(func(some_local_state))};

    do_something_in_current_thread();
}

int main()
{
    f();

    return 0;
}
