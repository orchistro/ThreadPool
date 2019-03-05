/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <functional>
#include <condition_variable>
#include <cstdint>
#include <future>

#include "ThreadPool.hpp"

class ThreadPoolLambda final : public ThreadPool<std::function<void(void)>>
{
    public:
        ThreadPoolLambda(const size_t aThrCnt) : ThreadPool<std::function<void(void)>>(aThrCnt)
        {
        }

        ThreadPoolLambda(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda(ThreadPoolLambda&&) = delete;

        ThreadPoolLambda& operator=(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda& operator=(ThreadPoolLambda&&) = delete;

        void executeTask(std::optional<std::function<void(void)>>&& aFunc) const override
        {
            aFunc.value()();
        }

        template <typename Func, typename ...ArgType>
        auto push(Func&& aFunc, ArgType&&... aArgs) -> std::future<decltype(aFunc(aArgs...))>
        {
            auto sFuncWithArgs = std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...);

            // Need to use shared_ptr to avoid compile error in clang:
            // https://bugs.llvm.org/show_bug.cgi?id=38325
            auto sPackage = std::make_shared<std::packaged_task<decltype(aFunc(aArgs...))(void)>>(std::move(sFuncWithArgs));
            auto sFuture{sPackage->get_future()};

            auto sTaskWrapper = [sPackage](void) -> void { (*sPackage)(); };

            mQueue.push(std::move(sTaskWrapper));

            mCond.notify_one();

            return sFuture;
        }
};

