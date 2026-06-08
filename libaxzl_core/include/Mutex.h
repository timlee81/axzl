/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.h"

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
        Lock();
    }

    void unlock()
    {
        Unlock();
    }

    void Lock()
    {
    }

    void Unlock()
    {
    }

private:
    /** Mutex name */
    std::string mName;
};

}