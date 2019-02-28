#include <chrono>

#include "ThreadPool.hpp"

int main(void)
{
    using namespace std::chrono_literals;

    ThreadPool tp(3);

    for (size_t i = 0; i < 3; i++)
    {
        std::this_thread::sleep_for(1s);
        std::cout << i << "\n";
    }

    tp.stop();

    return 0;
}
