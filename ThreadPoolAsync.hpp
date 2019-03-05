/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <functional>
#include <condition_variable>
#include <cstdint>
#include <future>

#include "ThreadPool.hpp"

template <typename Func, typename ...Args>
concept bool VoidFunc = requires(Func aFunc, Args... aArgs)
{
    { aFunc(aArgs...) } -> void;
};

class ThreadPoolAsync final : public ThreadPool<std::future<void>>
{
    public:
        ThreadPoolAsync(const size_t aThrCnt) : ThreadPool<std::future<void>>(aThrCnt)
        {
        }

        ThreadPoolAsync(const ThreadPoolAsync&) = delete;
        ThreadPoolAsync(ThreadPoolAsync&&) = delete;

        ThreadPoolAsync& operator=(const ThreadPoolAsync&) = delete;
        ThreadPoolAsync& operator=(ThreadPoolAsync&&) = delete;

        void executeTask(std::optional<std::future<void>>&& aFuture) const override
        {
            aFuture.value().get();
        }

        template <typename Func, typename ...ArgType> requires VoidFunc<Func, ArgType...>
        void push(Func&& aFunc, ArgType&&... aArgs)
        {
            auto sFuncWithArgs = std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...);

            auto sWrapper = [mFuncWithArgs{std::move(sFuncWithArgs)}] (void)
            {
                mFuncWithArgs();
            };

            mQueue.push(std::async(std::launch::deferred, std::move(sWrapper)));

            mCond.notify_one();
        }

        template <typename Func, typename ...ArgType>
        auto push(Func&& aFunc, ArgType&&... aArgs) -> std::future<decltype(aFunc(aArgs...))>
        {
            using RetType = decltype(aFunc(aArgs...));

            std::promise<RetType> sPromise;
            auto sFuture = sPromise.get_future();

            auto sFuncWithArgs = std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...);

            auto sWrapper = [mFuncWithArgs{std::move(sFuncWithArgs)}, mPromise{std::move(sPromise)}] (void) mutable
            {
                mPromise.set_value(mFuncWithArgs());
            };

            mQueue.push(std::async(std::launch::deferred, std::move(sWrapper)));

            mCond.notify_one();

            return sFuture;
        }
};

