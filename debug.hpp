/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#pragma once

#include <mutex>
#include <iostream>

inline std::mutex gOutMutex;

#define DEBUG(args) do \
{ \
    /* std::unique_lock l(gOutMutex);*/ \
    std::cout << args << "\n"; \
} while (0)

