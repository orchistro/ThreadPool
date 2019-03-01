#include <chrono>

#include "ThreadPool.hpp"

int main(void)
{
    using namespace std::chrono_literals;

    ThreadPool tp(3);

    auto f = [](int64_t id) -> void {
        DEBUG("BEGIN " << id);
        std::this_thread::sleep_for(std::chrono::milliseconds(id));
        DEBUG("END" << id);
    };

    tp.push(f);
    tp.push(f);
    tp.push(f);

    std::this_thread::sleep_for(10s);

    tp.stop();

    return 0;
}
