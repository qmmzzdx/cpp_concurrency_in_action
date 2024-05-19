#include <cstdio>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>

template <typename T>
class Singleton
{
private:
    Singleton() = default;
    Singleton(Singleton<T> const &other) = delete;
    Singleton &operator=(Singleton<T> const &other) = delete;
    static std::shared_ptr<T> instance;

public:
    static std::shared_ptr<T> GetInstance(T value)
    {
        static std::once_flag init_flag;
        std::call_once(init_flag, [&]() {
            instance = std::shared_ptr<T>(new T(value));
        });
        return instance;
    }

    static void PrintAddress()
    {
        printf("Value = %d\n", *instance);
    }

    ~Singleton()
    {
        printf("Singleton pointer destructed\n", *instance);
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance = nullptr;

int main()
{
    std::function<void(int)> test_singleton = [&](int value) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Singleton<int>::GetInstance(value);
        Singleton<int>::PrintAddress();
    };
    std::thread t1(test_singleton, 666);
    std::thread t2(test_singleton, 888);
    std::thread t3(test_singleton, 999);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
