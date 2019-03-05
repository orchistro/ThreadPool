/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <atomic>
#include <mutex>
#include <chrono>
#include <condition_variable>

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

