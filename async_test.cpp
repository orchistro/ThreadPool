#include <chrono>

#include "ThreadPoolAsync.hpp"

using namespace std::chrono_literals;

int64_t func1(const int32_t a, const int32_t b)
{
    DEBUG("func1(" << pthread_self() << ")" << "a:" << a << " b:" << b);
    std::this_thread::sleep_for(5s);
    return 100;
}

void func3(const int32_t a, const std::string& b)
{
    DEBUG("func3(" << pthread_self() << ")" << "a:" << a << " b:" << b);
    std::this_thread::sleep_for(5s);
}

int32_t main(void)
{
    ThreadPoolAsync sThreadPool(3);

    auto fut1 = sThreadPool.push(func1, 10, 20);
    auto fut2 = sThreadPool.push([](void)->int64_t {DEBUG("lambda1(" << pthread_self() << ")"); return 200;});
    auto fut3 = sThreadPool.push(func3, 14, "TESTSTRING");

    std::this_thread::sleep_for(2s);

    DEBUG("fut1:" << fut1.get());
    DEBUG("fut2:" << fut2.get());
    // DEBUG("fut3:" << fut3.get());

    sThreadPool.stop();

    return 0;
}


