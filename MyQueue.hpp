/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <queue>

template <typename T>
class MyQueue
{
    public:
        std::optional<T> pop(void)
        {
            std::unique_lock sLock(mMutex);

            if (mQueue.empty() == true)
            {
                return std::nullopt;
            }
            else
            {
                auto sElem = std::move(mQueue.front());
                mQueue.pop();
                return std::optional<T>(std::move(sElem));
            }
        }

        void push(T&& aElem)
        {
            std::unique_lock sLock(mMutex);
            mQueue.push(std::forward<T>(aElem));
        }

    private:
        std::queue<T> mQueue;
        std::mutex mMutex;
};

