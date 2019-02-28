/*
 * vim 설정
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <thread>
#include <vector>

template <typename T>
class TaskQueue
{
    public:
        TaskQueue()
        {
        }

        std::optional<T> pop(void)
        {
            std::unique_lock sLock(mMutex);

            if (mQueue.empty() == true)
            {
                return {};
            }
            else
            {
                auto sTask = mQueue.front();
                mQueue.pop();
                return sTask;
            }
        }

    private:
        std::queue<T> mQueue;
        std::mutex mMutex;
};

class ThreadPool
{
    private:
        std::vector<std::thread> mThrList;
        size_t mMinThreadCnt;
        size_t mMaxThreadCnt;

        TaskQueue<std::function<void(int)> mTaskQueue;

        std::mutex mMutex;
        std::condition_variable mCond;

        bool mRun = false;  // atomic할 필요가 있을까? 다른 코어에서 수행중인 쓰레드에는 전파가 안되나?

        auto mThrMain = [this](void)
        {
            while (this->mRun == true)
            {
                if (auto sTask = mQueue.pop(); sTask.has_value() == true)
                {
                    // call sTask
                    sTask();
                }
                else
                {
                    // wait for task
                    std::unique_lock sLock(this->mMutex);
                    mCond.wait(sLock);
                }
            }
        };

    public:
        ThreadPool(const size_t aMinThreadCnt, const size_t aMaxThreadCnt)
            : mThrList(aMaxThreadCnt), mMinThreadCnt(aMinThreadCnt), mMaxThreadCnt(aMaxThreadCnt)
        {
            for (size_t i = 0; i < aMinThreadCnt)
            {
                mThrList.emplace_back(mThrMain);
            }
        }

        template <typename T>
        void push(T&& aTask)
        {
            mTaskQueue.push(aTask);
        }

    private:



};

