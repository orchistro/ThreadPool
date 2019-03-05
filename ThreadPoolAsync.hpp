/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <thread>
#include <vector>
#include <iostream>
#include <optional>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <future>

#include <pthread.h>

#include "debug.hpp"
#include "ThreadStruct.hpp"
#include "MyQueue.hpp"

template <typename Func, typename ...Args>
concept bool VoidFunc = requires(Func aFunc, Args... aArgs)
{
    { aFunc(aArgs...) } -> void;
};

class ExecBarrier
{
    public:
        std::atomic<size_t> mCnt;
        const size_t mThreshold;

        std::mutex mMutex;
        std::condition_variable mCond;

        ExecBarrier(const size_t aThreshold) : mCnt(0), mThreshold(aThreshold)
        {
        }

        ExecBarrier(const ExecBarrier&) = delete;
        ExecBarrier(ExecBarrier&&) = delete;
        ExecBarrier& operator=(const ExecBarrier&) = delete;
        ExecBarrier& operator=(ExecBarrier&&) = delete;

    public:
        void waitOnArrive(void)
        {
            mCnt++;
            if (mCnt == mThreshold)
            {
                mCond.notify_all();
            }
            else
            {
                using namespace std::chrono_literals;
                std::unique_lock sBarrierLock(mMutex);
                mCond.wait(sBarrierLock);
            }
        }
};

class ThreadPoolAsync
{
    private:
        const size_t mThrCnt;
        ExecBarrier mBarrier;

        std::vector<ThreadStruct> mThrList;

        MyQueue<std::future<void>> mFutureQueue;

        std::mutex mMutex;
        std::condition_variable mCond;


        bool mRun = true;
        std::atomic<size_t> mStartCnt = 0;

    public:
        ThreadPoolAsync(const size_t aThrCnt) : mThrCnt(aThrCnt), mBarrier(aThrCnt + 1)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPoolAsync::threadMain, this);
            }

            mBarrier.waitOnArrive();
        }

        void threadMain(ThreadStruct* aStatus)
        {
            mBarrier.waitOnArrive();

            while (mRun == true)
            {
                if (auto sFuture = mFutureQueue.pop(); sFuture.has_value() == true)
                {
                    sFuture.value().get();
                }
                else
                {
                    using namespace std::chrono_literals;

                    std::unique_lock sLock(mMutex);
                    (void)mCond.wait_for(sLock, 100ms); // no need to prevent spurious wakeups, let it happen
                }
            }
        }

        void stop(void)
        {
            mRun = false;

            for (auto& sThr : mThrList)
            {
                sThr.mThread.join();
            }
        }

        template <typename Func, typename ...ArgType> requires VoidFunc<Func, ArgType...>
        void push(Func&& aFunc, ArgType&&... aArgs)
        {
            auto sFuncWithArgs = std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...);

            auto sWrapper = [mFuncWithArgs{std::move(sFuncWithArgs)}] (void)
            {
                mFuncWithArgs();
            };

            mFutureQueue.push(std::async(std::launch::deferred, std::move(sWrapper)));

            mCond.notify_one();
        }

        template <typename Func, typename ...ArgType>
        auto push(Func&& aFunc, ArgType&&... aArgs) -> std::future<decltype(aFunc(aArgs...))>
        {
            std::promise<decltype(aFunc(aArgs...))> sPromise;
            auto sFuture = sPromise.get_future();

            auto sFuncWithArgs = std::bind(std::forward<Func>(aFunc), std::forward<ArgType>(aArgs)...);

            auto sWrapper = [mFuncWithArgs{std::move(sFuncWithArgs)}, mPromise{std::move(sPromise)}] (void) mutable
            {
                mPromise.set_value(mFuncWithArgs());
            };

            mFutureQueue.push(std::async(std::launch::deferred, std::move(sWrapper)));

            mCond.notify_one();

            return sFuture;
        }

};

