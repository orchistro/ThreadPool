#include <chrono>

#include "ThreadPool.hpp"

int main(void)
{
    using namespace std::chrono_literals;

    ThreadPool tp(3);

    tp.push([](int){DEBUG("1begin");std::this_thread::sleep_for(1s);DEBUG("1end");return 1;});
    std::this_thread::sleep_for(200ms);
    tp.push([](int){DEBUG("2begin");std::this_thread::sleep_for(1.5s);DEBUG("2end");return 2;});

    for (size_t i = 0; i < 3; i++)
    {
        std::this_thread::sleep_for(1s);
    }

    tp.stop();

    return 0;
}
