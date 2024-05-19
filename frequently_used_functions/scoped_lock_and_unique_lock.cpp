#include <mutex>

class some_big_object
{
};

void swap(some_big_object &lhs, some_big_object &rhs)
{
}

class X
{
private:
    some_big_object some_detail;
    mutable std::mutex m;

public:
    X(some_big_object const &sd) : some_detail(sd) {}

    friend void swap1(X &lhs, X &rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        std::unique_lock<std::mutex> lka(lhs.m, std::defer_lock);
        std::unique_lock<std::mutex> lkb(rhs.m, std::defer_lock);
        std::lock(lka, lkb);
        swap(lhs.some_detail, rhs.some_detail);
    }

    friend void swap2(X &lhs, X &rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        std::scoped_lock guard(lhs.m, rhs.m);
        swap(lhs.some_detail, rhs.some_detail);
    }
};

int main()
{
    some_big_object s;
    X a(s), b(s);

    swap1(a, b);
    swap2(a, b);

    return 0;
}
