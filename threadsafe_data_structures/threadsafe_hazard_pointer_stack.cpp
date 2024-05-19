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
constexpr int max_hazard_pointers = 100;

struct hazard_pointer
{
    std::atomic<std::thread::id> id;
    std::atomic<void *> pt;
};
hazard_pointer hazard_pointers[max_hazard_pointers];

bool outstanding_hazard_pointers_for(void *p)
{
    for (int i = 0; i < max_hazard_pointers; ++i)
    {
        if (hazard_pointers[i].pt.load() == p)
        {
            return true;
        }
    }
    return false;
}

class hp_owner
{
private:
    hazard_pointer *hp;

public:
    hp_owner(hp_owner const &) = delete;
    hp_owner &operator=(hp_owner const &) = delete;

    hp_owner() : hp(nullptr)
    {
        for (int i = 0; i < max_hazard_pointers; ++i)
        {
            std::thread::id old_id;
            if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id()))
            {
                hp = &hazard_pointers[i];
                break;
            }
        }
        if (!hp)
        {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void *> &get_pointer()
    {
        return hp->pt;
    }

    ~hp_owner()
    {
        hp->pt.store(nullptr);
        hp->id.store(std::thread::id());
    }
};

std::atomic<void *> &get_hazard_pointer_for_current_thread()
{
    thread_local static hp_owner hazard;
    return hazard.get_pointer();
}

template <typename T>
class hazard_pointer_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node *next;
        node(T const &_data) : data(std::make_shared<T>(_data)), next(nullptr) {}
    };
    struct data_to_reclaim
    {
        node *reclaim_data;
        data_to_reclaim *next;
        data_to_reclaim(node *p) : reclaim_data(p), next(nullptr) {}
        ~data_to_reclaim() { delete reclaim_data; }
    };
    std::atomic<node *> head = nullptr;
    std::atomic<data_to_reclaim *> nodes_to_reclaim = nullptr;

    hazard_pointer_stack(const hazard_pointer_stack &) = delete;
    hazard_pointer_stack &operator=(const hazard_pointer_stack &) = delete;

    void add_to_reclaim_list(data_to_reclaim *nd)
    {
        nd->next = nodes_to_reclaim.load();
        while (!nodes_to_reclaim.compare_exchange_weak(nd->next, nd)) continue;
    }

    void reclaim_later(node *nd)
    {
        add_to_reclaim_list(new data_to_reclaim(nd));
    }

    void delete_nodes_with_no_hazards()
    {
        data_to_reclaim *cur = nodes_to_reclaim.exchange(nullptr);

        while (cur)
        {
            data_to_reclaim *const ne = cur->next;
            if (!outstanding_hazard_pointers_for(cur->reclaim_data))
            {
                delete cur;
            }
            else
            {
                add_to_reclaim_list(cur);
            }
            cur = ne;
        }
    }

public:
    hazard_pointer_stack() {}

    void push(T const &val)
    {
        node *const new_node = new node(val);
        new_node->next = head.load();
        while (!head.compare_exchange_weak(new_node->next, new_node)) continue;
    }

    std::shared_ptr<T> pop()
    {
        std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
        node *old_head = head.load();

        do
        {
            node *temp = nullptr;
            do
            {
                temp = old_head, hp.store(old_head);
                old_head = head.load();
            } while (old_head != temp);
        } while (old_head && !head.compare_exchange_strong(old_head, old_head->next));
        hp.store(nullptr);
        std::shared_ptr<T> res;
        if (old_head)
        {
            res.swap(old_head->data);
            if (outstanding_hazard_pointers_for(old_head))
            {
                reclaim_later(old_head);
            }
            else
            {
                delete old_head;
            }
            delete_nodes_with_no_hazards();
        }
        return res;
    }
};

int main()
{
    std::mutex mtx;
    std::unordered_set<int> rmset;
    hazard_pointer_stack<int> test_stack;

    std::thread t1([&]() {
        for (int i = 0; i < 10; ++i)
        {
            test_stack.push(i + 1);
            std::string str = "Push value: " + std::to_string(i + 1) + " to hazard_pointer_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from hazard_pointer_stack successfully.\n";
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
            std::string str = "Pop value: " + std::to_string(*hd) + " from hazard_pointer_stack successfully.\n";
            std::cout << str;
        }
    });

    const auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    t3.join();
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;
    std::cout << "Test hazard_pointer_stack " << (rmset.size() == 10 ? "successfully.\n" : "unsuccessfully.\n");
    std::cout << "Total consuming times: " << consumption.count() << "ms.\n";

    return 0;
}
