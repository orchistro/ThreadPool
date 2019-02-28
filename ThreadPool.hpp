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

    private:
        std::queue<Task> mQueue;
        std::mutex mMutex;
};

class ThreadStruct
{
    public:
        template <typename Func, typename... Args>
        ThreadStruct(const std::string& aName, Func&& aFunc, Args... args) : mName(aName), mThread(aFunc, args...)
        {
        }

    public:
        const std::string mName;
        std::thread mThread;
};

class ThreadPool
{
    private:
        size_t mThrCnt;
        std::vector<ThreadStruct> mThrList;

        TaskQueue<std::function<void(int)>> mTaskQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = true;  // asuming it is ok not to be atomic.

    public:
        ThreadPool(const size_t aThrCnt) : mThrCnt(aThrCnt)
        {
            mThrList.reserve(aThrCnt);

            for (size_t i = 0; i < aThrCnt; i++)
            {
                std::string sName{"Thr"};
                mThrList.emplace_back(sName + std::to_string(i), &ThreadPool::threadMain, this);
            }
        }

        void threadMain(void)
        {
            DEBUG("Thread " << std::this_thread::get_id());

            while (mRun == true)
            {
                if (auto sTask = mTaskQueue.pop(); sTask.has_value() == true)
                {
                    (*sTask)(0);
                }
                else
                {
                    using namespace std::chrono_literals;

                    std::unique_lock sLock(mMutex);
                    mCond.wait_for(sLock, 500ms, [this]{return this->mRun;});
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
                DEBUG("Joining " << sThr.mThread.get_id() << "(" << sThr.mName << ")");
                sThr.mThread.join();
            }
        }
};

