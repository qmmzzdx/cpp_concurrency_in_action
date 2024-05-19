#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class threadsafe_list
{
private:
    struct node
    {
        std::mutex mtx;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        node() : next() {}
        node(T const &value) : data(std::make_shared<T>(value)) {}
    };
    node head;

public:
    threadsafe_list() {}

    ~threadsafe_list()
    {
        remove_if([](T const &){ return true; });
    }

    threadsafe_list(threadsafe_list const &other) = delete;

    threadsafe_list &operator=(threadsafe_list const &other) = delete;

    void push_front(T const &value)
    {
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head.mtx);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template <typename Function>
    void for_each(Function f)
    {
        node *cur = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node *const ne = cur->next.get())
        {
            std::unique_lock<std::mutex> next_lk(ne->mtx);
            lk.unlock();
            f(*ne->data);
            cur = ne;
            lk = std::move(next_lk);
        }
    }

    template <typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p)
    {
        node *cur = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node *const ne = cur->next.get())
        {
            std::unique_lock<std::mutex> next_lk(ne->mtx);
            lk.unlock();
            if (p(*ne->data))
            {
                return ne->data;
            }
            cur = ne;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template <typename Predicate>
    void remove_if(Predicate p)
    {
        node *cur = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node *const ne = cur->next.get())
        {
            std::unique_lock<std::mutex> next_lk(ne->mtx);
            if (p(*ne->data))
            {
                std::unique_ptr<node> old_next = std::move(cur->next);
                cur->next = std::move(ne->next);
                next_lk.unlock();
            }
            else
            {
                lk.unlock(), cur = ne;
                lk = std::move(next_lk);
            }
        }
    }
};

int main()
{
    threadsafe_list<int> test_list;

    return 0;
}
