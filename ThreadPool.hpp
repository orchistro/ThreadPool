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

#include <sched.h>

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
        ThreadPool(const size_t aThrCnt, const bool aAffinity) : mThrCnt(aThrCnt), mBarrier(aThrCnt + 1)
        {
            uint32_t sNumCpus = std::thread::hardware_concurrency();

            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPool::threadMain, this);

                if (aAffinity == true)
                {
                    cpu_set_t sCpuSet;

                    CPU_ZERO(&sCpuSet);
                    CPU_SET((i % sNumCpus), &sCpuSet);

                    if (int32_t sRc = pthread_setaffinity_np(mThrList.back().mThread.native_handle(),
                                                             sizeof(cpu_set_t), &sCpuSet);
                        sRc != 0)
                    {
                        std::cerr << "Error calling pthread_setaffinity_np: " << sRc << "\n";
                        throw std::pair<int32_t, int32_t>(errno, sRc);
                    }
                }
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

            while (true)
            {
                if (auto sElem = mQueue.pop(); sElem.has_value() == true)
                {
                    executeTask(std::move(sElem));
                }
                else
                {
                    if (mRun == false)
                    {
                        break;
                    }
                    else
                    {
                        using namespace std::chrono_literals;
                        std::unique_lock sLock(mMutex);
                        (void)mCond.wait_for(sLock, 100ms); // no need to prevent spurious wakeups, let it happen
                    }
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

        virtual void executeTask(std::optional<QueElem>&& aElem) const = 0;
};

