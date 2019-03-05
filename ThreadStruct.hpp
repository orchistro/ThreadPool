/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <thread>
#include <utility>

class ThreadStruct
{
    public:
        std::thread mThread;

        enum class State
        {
            IDLE,
            RUNNING
        };

        State mState = State::IDLE;

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


