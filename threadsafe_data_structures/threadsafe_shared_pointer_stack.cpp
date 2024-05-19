#include <iostream>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>
#include <string>
#include <functional>
#include <unordered_set>

template <typename T>
class shared_pointer_atomic_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::shared_ptr<node> next;
        node(T const &_data) : data(std::make_shared<T>(_data)) {}
    };
    std::shared_ptr<node> head;

    shared_pointer_atomic_stack(const shared_pointer_atomic_stack &) = delete;
    shared_pointer_atomic_stack &operator=(const shared_pointer_atomic_stack &) = delete;

public:
    shared_pointer_atomic_stack() {}

    void push(T const &val)
    {
        std::shared_ptr<node> const new_node = std::make_shared<node>(val);
        new_node->next = std::atomic_load(&head);
        while (!std::atomic_compare_exchange_weak(&head, &new_node->next, new_node)) continue;
    }

    std::shared_ptr<T> pop()
    {
        std::shared_ptr<node> old_head = std::atomic_load(&head);
        while (old_head && !std::atomic_compare_exchange_weak(&head, &old_head, std::atomic_load(&old_head->next))) continue;
        if (old_head)
        {
            std::atomic_store(&old_head->next, std::shared_ptr<node>());
            return old_head->data;
        }
        return std::shared_ptr<T>();
    }

    ~shared_pointer_atomic_stack()
    {
        while (pop()) continue;
    }
};

int main()
{
    std::mutex mtx;
    std::unordered_set<int> rmset;
    shared_pointer_atomic_stack<int> test_stack;

    std::thread t1([&]() {
        for (int i = 0; i < 10; ++i)
        {
            test_stack.push(i + 1);
            std::string str = "Push value: " + std::to_string(i + 1) + " to shared_pointer_atomic_stack successfully.\n";
            std::cout << str;
        }
    });

    std::thread t2([&]() {
        while (rmset.size() < 10)
        {
            auto hd = test_stack.pop();
            if (!hd)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::lock_guard<std::mutex> lk(mtx);
            rmset.insert(*hd);
            std::string str = "Pop value: " + std::to_string(*hd) + " from shared_pointer_atomic_stack successfully.\n";
            std::cout << str;
        }
    });

    std::thread t3([&]() {
        while (rmset.size() < 10)
        {
            auto hd = test_stack.pop();
            if (!hd)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::lock_guard<std::mutex> lk(mtx);
            rmset.insert(*hd);
            std::string str = "Pop value: " + std::to_string(*hd) + " from shared_pointer_atomic_stack successfully.\n";
            std::cout << str;
        }
    });

    const auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    t3.join();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;
    std::cout << "Test shared_pointer_atomic_stack " << (rmset.size() == 10 ? "successfully.\n" : "unsuccessfully.\n");
    std::cout << "Total consuming times: " << consumption.count() << "ms.\n";

    return 0;
}
