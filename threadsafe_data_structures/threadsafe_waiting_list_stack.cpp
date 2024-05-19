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
class lock_free_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node *next;
        node(T const &_data) : data(std::make_shared<T>(_data)), next(nullptr) {}
    };
    std::atomic<node *> head = nullptr;
    std::atomic<unsigned int> threads_in_pop = 0;
    std::atomic<node *> to_be_deleted = nullptr;

    lock_free_stack(const lock_free_stack &) = delete;
    lock_free_stack &operator=(const lock_free_stack &) = delete;

    static void delete_nodes(node *nodes)
    {
        while (nodes)
        {
            node *ne = nodes->next;
            delete nodes;
            nodes = ne;
        }
    }

    void chain_pending_nodes(node *nodes)
    {
        node *last = nodes;
        while (node *const ne = last->next)
        {
            last = ne;
        }
        chain_pending_nodes(nodes, last);
    }

    void chain_pending_nodes(node *first, node *last)
    {
        if (first && last)
        {
            last->next = to_be_deleted;
            while (!to_be_deleted.compare_exchange_weak(last->next, first)) continue;
        }
    }

    void chain_pending_node(node *nd)
    {
        chain_pending_nodes(nd, nd);
    }

    void try_reclaim(node *old_head)
    {
        if (threads_in_pop == 1)
        {
            node *nodes_to_delete = to_be_deleted.exchange(nullptr);
            if (--threads_in_pop == 0)
            {
                delete_nodes(nodes_to_delete);
            }
            else if (nodes_to_delete)
            {
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        }
        else
        {
            chain_pending_node(old_head);
            --threads_in_pop;
        }
    }

public:
    lock_free_stack() {}

    void push(T const &val)
    {
        node *const new_node = new node(val);
        new_node->next = head.load();
        while (!head.compare_exchange_weak(new_node->next, new_node)) continue;
    }

    std::shared_ptr<T> pop()
    {
        ++threads_in_pop;
        node *old_head = head.load();
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next)) continue;
        std::shared_ptr<T> res;
        if (old_head)
        {
            res.swap(old_head->data);
        }
        try_reclaim(old_head);
        return res;
    }
};

int main()
{
    std::mutex mtx;
    std::unordered_set<int> rmset;
    lock_free_stack<int> test_stack;

    std::thread t1([&]() {
        for (int i = 0; i < 10; ++i)
        {
            test_stack.push(i + 1);
            std::string str = "Push value: " + std::to_string(i + 1) + " to lock_free_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from lock_free_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from lock_free_stack successfully.\n";
            std::cout << str;
        }
    });

    const auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    t3.join();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;
    std::cout << "Test lock_free_stack " << (rmset.size() == 10 ? "successfully.\n" : "unsuccessfully.\n");
    std::cout << "Total consuming times: " << consumption.count() << "ms.\n";

    return 0;
}
