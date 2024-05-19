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
class ref_atomic_stack
{
private:
    struct node;
    struct counted_node_ptr
    {
        int external_count;
        node *ptr;
        counted_node_ptr() : external_count(0), ptr(nullptr) {}
    };
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        node(T const &val) : data(std::make_shared<T>(val)), internal_count(0) {}
    };
    std::atomic<counted_node_ptr> head;

    void increase_head_count(counted_node_ptr &old_counter)
    {
        counted_node_ptr new_counter;

        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!head.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

public:
    ref_atomic_stack()
    {
        counted_node_ptr head_node_ptr;
        head.store(head_node_ptr);
    }

    ~ref_atomic_stack()
    {
        while (pop()) continue;
    }

    void push(T const &val)
    {
        counted_node_ptr new_node;
        new_node.ptr = new node(val);
        new_node.external_count = 1;
        new_node.ptr->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(new_node.ptr->next, new_node, std::memory_order_release, std::memory_order_relaxed)) continue;
    }

    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);
        for (;;)
        {
            increase_head_count(old_head);
            node *const ptr = old_head.ptr;
            if (!ptr)
            {
                return std::shared_ptr<T>();
            }
            if (head.compare_exchange_strong(old_head, ptr->next, std::memory_order_relaxed))
            {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                int const count_increase = old_head.external_count - 2;
                if (ptr->internal_count.fetch_add(count_increase, std::memory_order_release) == -count_increase)
                {
                    delete ptr;
                }
                return res;
            }
            else if (ptr->internal_count.fetch_add(-1, std::memory_order_relaxed) == 1)
            {
                ptr->internal_count.load(std::memory_order_acquire);
                delete ptr;
            }
        }
    }
};

int main()
{
    std::mutex mtx;
    std::unordered_set<int> rmset;
    ref_atomic_stack<int> test_stack;

    std::thread t1([&]() {
        for (int i = 0; i < 10; ++i)
        {
            test_stack.push(i + 1);
            std::string str = "Push value: " + std::to_string(i + 1) + " to ref_atomic_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from ref_atomic_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from ref_atomic_stack successfully.\n";
            std::cout << str;
        }
    });

    const auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    t3.join();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;
    std::cout << "Test ref_atomic_stack " << (rmset.size() == 10 ? "successfully.\n" : "unsuccessfully.\n");
    std::cout << "Total consuming times: " << consumption.count() << "ms.\n";

    return 0;
}
