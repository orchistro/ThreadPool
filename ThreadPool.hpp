/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once    // should be ok with both llmv and gcc

#include <thread>
#include <vector>
#include <iostream>
#include <queue>
#include <optional>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>

inline std::mutex gOutMutex;

#define DEBUG(args) do \
{ \
    std::unique_lock l(gOutMutex); \
    std::cout << args << "\n"; \
} while (0)

template <typename Task>
class TaskQueue
{
    public:
        std::optional<Task> pop(void)
        {
            std::unique_lock sLock(mMutex);

            if (mQueue.empty() == true)
            {
                return std::nullopt;
            }
            else
            {
                auto sTask = mQueue.front();
                mQueue.pop();
                return sTask;
            }
        }

        void push(Task&& aTask)
        {
            std::unique_lock sLock(mMutex);

            mQueue.push(std::forward<Task>(aTask));
        }

    private:
        std::queue<Task> mQueue;
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

class ThreadPool
{
    private:
        const size_t mThrCnt;
        std::vector<ThreadStruct> mThrList;

        TaskQueue<std::function<void(int)>> mTaskQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = true;  // asuming it is ok not to be atomic.

        std::atomic<size_t> mRunningCnt = 0;

    public:
        ThreadPool(const size_t aThrCnt) : mThrCnt(aThrCnt)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                mThrList.emplace_back(&ThreadPool::threadMain, this);
            }
        }

        void threadMain(ThreadStruct* aStatus)
        {
            DEBUG("Thread " << std::this_thread::get_id());

            while (mRun == true)
            {
                if (auto sTask = mTaskQueue.pop(); sTask.has_value() == true)
                {
                    aStatus->mState = ThreadState::RUNNING;
                    mRunningCnt++;
                    (*sTask)(0);
                    mRunningCnt--;
                    aStatus->mState = ThreadState::IDLE;
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

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;

        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        void stop(void)
        {
            mRun = false;

            for (auto& sThr : mThrList)
            {
                DEBUG("Joining " << sThr.mThread.get_id());
                sThr.mThread.join();
            }
        }

        void push(std::function<void(int)>&& aFunc)
        {
            mTaskQueue.push(std::forward<std::function<void(int)>>(aFunc));
            mCond.notify_one();
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

