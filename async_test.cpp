#include <chrono>

#include "ThreadPoolAsync.hpp"

int64_t func1(void)
{
    DEBUG("func1");
    return 100;
}

int32_t main(void)
{
    using namespace std::chrono_literals;

    ThreadPoolAsync sThreadPool(3);

    sThreadPool.push(func1);
    sThreadPool.push([]()->int64_t {DEBUG("lambda1"); return 200;});

    std::this_thread::sleep_for(2s);

    sThreadPool.stop();

    return 0;
}


