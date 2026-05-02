/*
 * SPDX-FileCopyrightText: 2026 Tim Lee
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <string>

#if defined(_POSIX_THREADS)
#include <pthread.h>
#endif

namespace Axzl
{
/**
 * Implement Lockable
 *    Also should implement TryLockable and TimedLockable?
 */
class Mutex
{
public:
    // Deleted
    Mutex& operator=(Mutex& assign) = delete;
    Mutex& operator=(Mutex&& move) = delete;
    Mutex(Mutex& copy) = delete;
    Mutex(Mutex&& move) = delete;

    Mutex(const char* name)
        : mName(name)
    {
    }

    void lock()
    {
    }

    void unlock()
    {
    }

private:
    /** Mutex name */
    std::string mName;
};

}