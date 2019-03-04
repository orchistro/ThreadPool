/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <thread>
#include <vector>
#include <iostream>
#include <queue>
#include <optional>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <future>

#include <pthread.h>

#include "debug.hpp"

template <typename Func, typename ...Args>
concept bool VoidFunc = requires(Func f, Args... args)
{
    { f(args...) } -> void;
};

template <typename Future>
class FutureQueue
{
    public:
        std::optional<Future> pop(void)
        {
            std::unique_lock sLock(mMutex);

            if (mQueue.empty() == true)
            {
                return std::nullopt;
            }
            else
            {
                auto sFuture = std::move(mQueue.front());
                mQueue.pop();
                return std::optional<Future>(std::move(sFuture));
            }
        }

        void push(Future&& aFuture)
        {
            std::unique_lock sLock(mMutex);

            mQueue.push(std::forward<Future>(aFuture));
        }

    private:
        std::queue<Future> mQueue;
        std::mutex mMutex;
};

enum class ThreadState
{
    IDLE,
    RUNNING
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

class ThreadStruct
{
    public:
        std::thread mThread;
        ThreadState mState = ThreadState::IDLE;

    public:
        template <typename Func, typename... Args>
        ThreadStruct(Func&& aFunc, Args&&... aArgs)
            : mThread(std::forward<Func>(aFunc), std::forward<Args>(aArgs)..., this)
        {
        }

        ThreadStruct(const ThreadStruct&) = delete;
        ThreadStruct& operator=(const ThreadStruct&) = delete;

        ThreadStruct(ThreadStruct&&) = default;
        ThreadStruct& operator=(ThreadStruct&&) = default;
};

class ThreadPoolAsync
{
    private:
        const size_t mThrCnt;
        ExecBarrier mBarrier;

        std::vector<ThreadStruct> mThrList;

        FutureQueue<std::future<void>> mFutureQueue;

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
                    (void)mCond.wait_for(sLock, 100ms);
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
                if constexpr (std::is_void_v<decltype(aFunc(aArgs...))> == true)
                {
                    mFuncWithArgs();
                }
                else
                {
                    mPromise.set_value(mFuncWithArgs());
                }
            };

            mFutureQueue.push(std::async(std::launch::deferred, std::move(sWrapper)));

            mCond.notify_one();

            return sFuture;
        }

};

