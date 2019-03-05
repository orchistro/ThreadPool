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
#include "MyQueue.hpp"
#include "ThreadStruct.hpp"

class ThreadPoolLambda
{
    private:
        const size_t mThrCnt;
        std::vector<ThreadStruct> mThrList;

        MyQueue<std::function<void(void)>> mTaskQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = true;  // asuming it is ok not to be atomic.

        std::atomic<size_t> mRunningCnt = 0;

    public:
        ThreadPoolLambda(const size_t aThrCnt) : mThrCnt(aThrCnt)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPoolLambda::threadMain, this);
            }
        }

        void threadMain(ThreadStruct* aStatus)
        {
            DEBUG("Thread " << std::this_thread::get_id());

            while (mRun == true)
            {
                if (auto sTaskWrapper = mTaskQueue.pop(); sTaskWrapper.has_value() == true)
                {
                    aStatus->mState = ThreadStruct::State::RUNNING;
                    mRunningCnt++;
                    (*sTaskWrapper)();
                    mRunningCnt--;
                    aStatus->mState = ThreadStruct::State::IDLE;
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

        ThreadPoolLambda(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda(ThreadPoolLambda&&) = delete;

        ThreadPoolLambda& operator=(const ThreadPoolLambda&) = delete;
        ThreadPoolLambda& operator=(ThreadPoolLambda&&) = delete;

        void stop(void)
        {
            mRun = false;

            for (auto& sThr : mThrList)
            {
                DEBUG("Joining " << sThr.mThread.get_id());
                sThr.mThread.join();
            }
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

            mTaskQueue.push(std::move(sTaskWrapper));
            mCond.notify_one();

            return sFuture;
        }

        const size_t size(void) const
        {
            return mThrCnt;
        }

        const size_t getIdleCount(void) const
        {
            return mThrCnt - mRunningCnt;
        }
        const size_t getRunningCount(void) const
        {
            return mRunningCnt;
        }
};

