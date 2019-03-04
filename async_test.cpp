#include <chrono>

#include "ThreadPoolAsync.hpp"

using namespace std::chrono_literals;

int64_t func1(const int32_t a, const int32_t b)
{
    DEBUG("func1(" << pthread_self() << ")" << "a:" << a << " b:" << b);
    std::this_thread::sleep_for(5s);
    return 100;
}

int32_t main(void)
{
    ThreadPoolAsync sThreadPool(3);

    sThreadPool.push(func1, 10, 20);
    sThreadPool.push([](void)->int64_t {DEBUG("lambda1(" << pthread_self() << ")"); return 200;});

    std::this_thread::sleep_for(2s);

    sThreadPool.stop();

    return 0;
}


