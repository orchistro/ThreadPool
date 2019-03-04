#include <chrono>

#include "ThreadPool.hpp"

static std::string func4(int32_t a, int32_t b)
{
    DEBUG("BEGIN func4");
    DEBUG("END func4");
    return std::to_string(a + b);
}

static int64_t func5(int32_t a, int32_t b, int32_t c, std::string* aOutputStr)
{
    DEBUG("BEGIN func5");
    int64_t res = a * b * c;
    *aOutputStr = std::to_string(res);
    DEBUG("END func5");
    return res;
}

void func6(void)
{
    DEBUG("BEGIN func6");
    DEBUG("END func6");
}

static int64_t func8(int32_t a, int32_t b, int32_t c, std::string& aOutputStr)
{
    DEBUG("BEGIN func8");
    int64_t res = a * b * c;
    aOutputStr = "output:" + std::to_string(res);
    DEBUG("END func8");
    return res;
}

int main(void)
{
    using namespace std::chrono_literals;

    ThreadPool tp(3);

    auto lambda1 = [](int64_t id) -> int64_t
    {
        DEBUG("BEGIN " << id);
        std::this_thread::sleep_for(std::chrono::milliseconds(id));
        DEBUG("END" << id);
        return id;
    };

    std::future<int64_t> f1 = tp.push(lambda1, 10);
    std::future<int64_t> f2 = tp.push(lambda1, 20);
    std::future<int64_t> f3 = tp.push(lambda1, 30);

    // compilation error. culprit: return type
    std::future<std::string> f4 = tp.push(func4, 20, 10);

    std::string s;
    auto f5 = tp.push(func5, 30, 20, 10, &s);

    // compilation error. culprit: return type
    std::future<void> f6 = tp.push(func6);

    auto f7 = tp.push([](void) -> int64_t {DEBUG("func7");return 100;});

    std::string s2;
    auto f8 = tp.push(func8, 30, 20, 10, std::ref(s2));

    std::this_thread::sleep_for(10s);

    tp.stop();

    DEBUG("f1:" << f1.get());
    DEBUG("f2:" << f2.get());
    DEBUG("f3:" << f3.get());
    DEBUG("f4:" << f4.get());
    DEBUG("f5:" << f5.get());
    DEBUG("s:" << s);
    DEBUG("f7:" << f7.get());
    DEBUG("f8:" << f8.get());
    DEBUG("s2:" << s2);

    return 0;
}
