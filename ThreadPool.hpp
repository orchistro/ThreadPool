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
#include <cstdint>
#include <future>

#include <pthread.h>

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
                auto sTask = std::move(mQueue.front());
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

        TaskQueue<std::function<void(void)>> mTaskQueue;

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
                if (auto sTaskWrapper = mTaskQueue.pop(); sTaskWrapper.has_value() == true)
                {
                    aStatus->mState = ThreadState::RUNNING;
                    mRunningCnt++;
                    (*sTaskWrapper)();
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

