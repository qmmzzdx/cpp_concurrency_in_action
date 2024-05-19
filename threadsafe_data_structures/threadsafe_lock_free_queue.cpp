#include <iostream>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>
#include <string>
#include <exception>
#include <functional>
#include <unordered_set>

template <typename T>
class lock_free_queue
{
private:
    struct node;
    struct node_counter
    {
        unsigned int internal_count : 30;
        unsigned int external_counters : 2;
    };
    struct counted_node_ptr
    {
        int external_count;
        node *ptr;
    };
    struct node
    {
        std::atomic<T *> data;
        std::atomic<node_counter> count;
        std::atomic<counted_node_ptr> next;
        node() : data(nullptr)
        {
            node_counter new_count;
            counted_node_ptr node_ptr;
            
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);
            node_ptr.external_count = 0;
            node_ptr.ptr = nullptr;
            next.store(node_ptr);
        }
        void release_ref()
        {
            node_counter old_counter = count.load(std::memory_order_relaxed);
            node_counter new_counter;
            do
            {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while (!count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
            if (new_counter.internal_count == 0 && new_counter.external_counters == 0)
            {
                delete this;
            }
        }
    };
    std::atomic<counted_node_ptr> head;
    std::atomic<counted_node_ptr> tail;

    void increase_external_count(std::atomic<counted_node_ptr> &counter, counted_node_ptr &old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        }
        while(!counter.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

    void free_external_counter(counted_node_ptr &old_node)
    {
        node *const ptr = old_node.ptr;
        int const count_increase = old_node.external_count - 2;
        node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
        node_counter new_counter;

        do
        {
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
        if (new_counter.internal_count == 0 && new_counter.external_counters == 0)
        {
            delete ptr;
        }
    }

    void set_new_tail(counted_node_ptr &old_tail, counted_node_ptr const &new_tail)
    {
        node *const current_tail_ptr = old_tail.ptr;
        while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr) continue;
        if (old_tail.ptr == current_tail_ptr)
        {
            free_external_counter(old_tail);
            return;
        }
        current_tail_ptr->release_ref();
    }

public:
    lock_free_queue()
    {
        counted_node_ptr new_next;
        new_next.ptr = new node();
        new_next.external_count = 1;
        head.store(new_next), tail.store(new_next);
    }

    ~lock_free_queue()
    {
        while (pop()) continue;
        auto head_counted_node = head.load();
        delete head_counted_node.ptr;
    }

    void push(T new_value)
    {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        counted_node_ptr old_tail = tail.load();

        new_next.ptr = new node;
        new_next.external_count = 1;
        for (;;)
        {
            increase_external_count(tail, old_tail);
            T *old_data = nullptr;
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get()))
            {
                counted_node_ptr old_next = {0};
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next))
                {
                    delete new_next.ptr;
                    new_next = old_next;
                }
                set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            }
            else
            {
                counted_node_ptr old_next = {0};
                if (old_tail.ptr->next.compare_exchange_strong(old_next, new_next))
                {
                    old_next = new_next;
                    new_next.ptr = new node;
                }
                set_new_tail(old_tail, old_next);
            }
        }
    }

    std::unique_ptr<T> pop()
    {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);

        for (;;)
        {
            increase_external_count(head, old_head);
            node *const ptr = old_head.ptr;
            if (ptr == tail.load().ptr)
            {
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            counted_node_ptr ne = ptr->next.load();
            if (head.compare_exchange_strong(old_head, ne))
            {
                T *const res = ptr->data.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }
};

int main()
{
    std::mutex mtx;
    std::unordered_set<int> rmset;
    lock_free_queue<int> test_queue;

    std::thread t1([&]() {
        for (int i = 0; i < 10; ++i)
        {
            test_queue.push(i + 1);
            std::string str = "Push value: " + std::to_string(i + 1) + " to lock_free_queue successfully.\n";
            std::cout << str;
        }
    });

    std::thread t2([&]() {
        while (rmset.size() < 10)
        {
            auto hd = test_queue.pop();
            if (!hd)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::lock_guard<std::mutex> lk(mtx);
            rmset.insert(*hd);
            std::string str = "Pop value: " + std::to_string(*hd) + " from lock_free_queue successfully.\n";
            std::cout << str;
        }
    });

    std::thread t3([&]() {
        while (rmset.size() < 10)
        {
            auto hd = test_queue.pop();
            if (!hd)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            std::lock_guard<std::mutex> lk(mtx);
            rmset.insert(*hd);
            std::string str = "Pop value: " + std::to_string(*hd) + " from lock_free_queue successfully.\n";
            std::cout << str;
        }
    });

    const auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    t3.join();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;
    std::cout << "Test lock_free_queue " << (rmset.size() == 10 ? "successfully.\n" : "unsuccessfully.\n");
    std::cout << "Total consuming times: " << consumption.count() << "ms.\n";

    return 0;
}
