#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

std::atomic<int> count;
std::vector<int> queue_data;

void wait_for_more_items()
{
    std::cout << "wait_for_more_items...\n";
}

void process(int data)
{
    std::cout << "processing " << data << "...\n";
}

void populate_queue()
{
    constexpr int number_of_items = 20;
    queue_data.clear();

    for (int i = 0; i < number_of_items; ++i)
    {
        queue_data.push_back(i);
    }
    count.store(number_of_items, std::memory_order_release);
}

void consume_queue_items()
{
    while (true)
    {
        int idx = count.fetch_sub(1, std::memory_order_acquire);
        if (idx <= 0)
        {
            wait_for_more_items();
            continue;
        }
        process(queue_data[idx - 1]);
    }
}

int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);

    a.join();
    b.join();
    c.join();

    return 0;
}
