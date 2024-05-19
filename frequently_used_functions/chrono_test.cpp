#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    const auto now_time = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now_time);

    std::cout << "Current time = " << std::ctime(&t);

    const auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> consumption = end - start;

    std::cout << "Sleep(2s) = " << consumption.count() << "ms" << std::endl;

    std::chrono::milliseconds test_ms(54802);
    std::chrono::seconds test_s = std::chrono::duration_cast<std::chrono::seconds>(test_ms);

    std::cout << "test_ms(54802ms) = test_s(" << test_s.count() << "s)" << std::endl;

    return 0;
}
