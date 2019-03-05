/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <functional>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <memory>

#include "ThreadPool.hpp"

struct ITaskWrapper
{
    virtual void operator()(void) = 0;
};

template <typename Func>
class TaskWrapper : public ITaskWrapper
{
    public:
        TaskWrapper(Func&& aFunc) : mFunc(std::forward<Func>(aFunc))
        {
        }

        void operator()()
        {
            mFunc();
        }

    private:
        Func mFunc;
};

using TaskObjType = std::unique_ptr<ITaskWrapper>;

class ThreadPoolLambda final : public ThreadPool<TaskObjType>
{
    public:
        ThreadPoolLambda(const size_t aThrCnt) : ThreadPool<TaskObjType>(aThrCnt)
        {
        }

        ThreadPoolLambda(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda(ThreadPoolLambda&&) = delete;

        ThreadPoolLambda& operator=(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda& operator=(ThreadPoolLambda&&) = delete;

        void executeTask(std::optional<TaskObjType>&& aFunc) const override
        {
            (*aFunc.value())();
        }

        template <typename Func, typename ...ArgType>
        auto push(Func&& aFunc, ArgType&&... aArgs) -> std::future<decltype(aFunc(aArgs...))>
        {
            using RetType = decltype(aFunc(aArgs...));
            using PkgType = std::packaged_task<RetType(void)>;

            auto sFuncWithArgs{std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...)};
            auto sPackage{PkgType(std::move(sFuncWithArgs))};
            auto sFuture{sPackage.get_future()};
            auto sTaskWrapper{std::make_unique<TaskWrapper<PkgType>>(std::move(sPackage))};

            mQueue.push(std::move(sTaskWrapper));

            mCond.notify_one();

            return sFuture;
        }
};

