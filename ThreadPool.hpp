/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <thread>
#include <vector>
#include <optional>
#include <mutex>
#include <condition_variable>

#include <cstdint>

#include "debug.hpp"
#include "ThreadStruct.hpp"
#include "MyQueue.hpp"
#include "ExecBarrier.hpp"

template <typename QueElem>
class ThreadPool
{
    private:
        const size_t mThrCnt;
        ExecBarrier mBarrier;

        std::vector<ThreadStruct> mThrList;

    protected:
        MyQueue<QueElem> mQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = true;  // it is ok not to be atomic.

    public:
        ThreadPool(const size_t aThrCnt) : mThrCnt(aThrCnt), mBarrier(aThrCnt + 1)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPool::threadMain, this);
            }

            mBarrier.waitOnArrive();
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;

        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        void threadMain(ThreadStruct* aStatus)
        {
            mBarrier.waitOnArrive();

            while (mRun == true)
            {
                if (auto sElem = mQueue.pop(); sElem.has_value() == true)
                {
                    executeTask(std::move(sElem));
                }
                else
                {
                    using namespace std::chrono_literals;
                    std::unique_lock sLock(mMutex);
                    (void)mCond.wait_for(sLock, 100ms); // no need to prevent spurious wakeups, let it happen
                }
            }

            // TODO: Need to work on the case where mRun is set to false while mQueue is NOT empty
            //       In other words, need to process remaining queue elements.
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

        virtual void executeTask(std::optional<QueElem>&& aElem) const = 0;
};

