#include <chrono>

#include "ThreadPool.hpp"

int main(void)
{
    using namespace std::chrono_literals;

    ThreadPool tp(3);

    auto f = [](int64_t id) -> int64_t
    {
        DEBUG("BEGIN " << id);
        std::this_thread::sleep_for(std::chrono::milliseconds(id));
        DEBUG("END" << id);
        return id;
    };

    std::future<int64_t> f1 = tp.push(f, 10);
    std::future<int64_t> f2 = tp.push(f, 20);
    std::future<int64_t> f3 = tp.push(f, 30);

    std::this_thread::sleep_for(10s);

    tp.stop();

    DEBUG("f1:" << f1.get());
    DEBUG("f2:" << f2.get());
    DEBUG("f3:" << f3.get());

    return 0;
}
