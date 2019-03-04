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
        std::vector<ThreadStruct> mThrList;

        FutureQueue<std::future<int64_t>> mFutureQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = true;

    public:
        ThreadPoolAsync(const size_t aThrCnt) : mThrCnt(aThrCnt)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPoolAsync::threadMain, this);
            }
        }

        void threadMain(ThreadStruct* aStatus)
        {
            DEBUG("Thread " << std::this_thread::get_id());

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
                    mCond.wait_for(sLock, 600ms, [this]{return (this->mRun == false);});
                }
            }

            DEBUG("End " << std::this_thread::get_id());
        }

        void stop(void)
        {
            mRun = false;

            for (auto& sThr : mThrList)
            {
                DEBUG("Joining " << sThr.mThread.get_id());
                sThr.mThread.join();
            }
        }

        void push(std::function<int64_t(void)>&& aFunc)
        {
            mFutureQueue.push(std::async(std::launch::deferred, aFunc));
        }
};