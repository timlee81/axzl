/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.h"

#include <pthread.h>
#include <string>

namespace Axzl
{
/**
 * Implement Lockable
 *
 *    Also should implement TryLockable and TimedLockable
 */
class Mutex
{
public:
    // Deleted
    Mutex& operator=(Mutex& assign) = delete;
    Mutex& operator=(Mutex&& move) = delete;
    Mutex(Mutex& copy) = delete;
    Mutex(Mutex&& move) = delete;

    // SHOULD I JUST ADD PTHREAD header types

    Mutex(const char* name)
    : mName(name)
    , mLog(nullptr)
    {
    }

    Mutex(const char* name, std::shared_ptr<Log>& log, int type = PTHREAD_MUTEX_NORMAL, int flags)
    : mName(name)
    , mLog(log)
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
        int pthread_mutex_lock()
    }

    void Unlock()
    {
    }

private:
    void CreateMutex();

    /** Mutex name */
    std::string mName;

    /** Log interface */
    std::shared_ptr<Log> mLog;

#if defined(_POSIX_THREADS)
    pthread_mutex_t mMtx;
#endif
};
}

#include <cerrno>
#include <pthread.h>
#include <system_error>

/**
 * Mutex Attribute values
 */
enum class MutexType
{
    Normal = PTHREAD_MUTEX_NORMAL,
    ErrorCheck = PTHREAD_MUTEX_ERRORCHECK,
    Recursive = PTHREAD_MUTEX_RECURSIVE,
    Default = PTHREAD_MUTEX_DEFAULT
};

enum class MutexShared
{
    Private = PTHREAD_PROCESS_PRIVATE,
    Shared = PTHREAD_PROCESS_SHARED
};

enum class MutexRobust
{
    Stalled = PTHREAD_MUTEX_STALLED,
    Robust = PTHREAD_MUTEX_ROBUST
};

enum class MutexProtocol
{
    None = PTHREAD_PRIO_NONE,
    Inherit = PTHREAD_PRIO_INHERIT,
    Protect = PTHREAD_PRIO_PROTECT
};

/**
 * Attribute Object
 */
template <
    MutexType Type = MutexType::Normal,
    MutexShared Shared = MutexShared::Private,
    MutexRobust Robust = MutexRobust::Stalled,
    MutexProtocol Proto = MutexProtocol::None,
    int Ceiling = 0>
struct MutexAttrib
{
    static constexpr MutexType type = Type;
    static constexpr MutexShared shared = Shared;
    static constexpr MutexRobust robust = Robust;
    static constexpr MutexProtocol proto = Proto;
    static constexpr int priority_ceiling = Ceiling;
};

// ============================================================================
// 3. COMMONLY USED CONFIGURATION ALIASES ("USING" STATEMENTS)
// ============================================================================
// Standard thread synchronization within a single process
using DefaultConfig = MutexAttrib<>;

// Allows a single thread to safely relock the same mutex recursively
using RecursiveConfig = MutexAttrib<MutexType::Recursive>;

// Safe debugging attribute that returns an error code on deadlocks or improper unlocks
using ErrorCheckConfig = MutexAttrib<MutexType::ErrorCheck>;

// Inter-process communication attribute (for use in shared memory)
using SharedConfig = MutexAttrib<MutexType::Normal, MutexShared::Shared>;

// Inter-process communication that safely reports if a process crashes while holding the lock
using RobustSharedConfig = MutexAttrib<MutexType::Normal, MutexShared::Shared, MutexRobust::Robust>;

// ============================================================================
// 4. UNIFIED, NON-TEMPLATED MUTEX WRAPPER CLASS
// ============================================================================
class Mutex
{
public:
    // Templated constructor maps static configuration fields at compile-time
    template <typename Attribute>
    explicit Mutex(Attribute /*tag*/)
    {
        pthread_mutexattr_t attr;

        int ret = pthread_mutexattr_init(&attr);
        if (ret != 0)
        {
            throw std::system_error(ret, std::generic_category(), "Failed to init attributes");
        }

        // Apply properties using compile-time static constants
        pthread_mutexattr_settype(&attr, static_cast<int>(Attribute::type));
        pthread_mutexattr_setpshared(&attr, static_cast<int>(Attribute::shared));
        pthread_mutexattr_setrobust(&attr, static_cast<int>(Attribute::robust));
        pthread_mutexattr_setprotocol(&attr, static_cast<int>(Attribute::proto));

        if (Profile::proto == MutexProtocol::Protect)
        {
            pthread_mutexattr_setprioceiling(&attr, Profile::priority_ceiling);
        }

        // Initialize actual mutex using configured attributes
        ret = pthread_mutex_init(&mutex_, &attr);

        // Clean up attribute structural data immediately
        pthread_mutexattr_destroy(&attr);

        if (ret != 0)
        {
            throw std::system_error(ret, std::generic_category(), "Failed to init mutex");
        }
    }

    // Default constructor uses the DefaultConfig automatically
    Mutex()
    : Mutex(DefaultConfig { })
    {
    }

    // Destructor manages resources via RAII
    ~Mutex()
    {
        pthread_mutex_destroy(&mutex_);
    }

    // Explicitly disable copy semantics for safety
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    // Standard Lock operation
    void lock()
    {
        int ret = pthread_mutex_lock(&mutex_);
        if (ret != 0)
        {
            // Special error check hook if you choose to handle EOWNERDEAD later
            throw std::system_error(ret, std::generic_category(), "Failed to lock mutex");
        }
    }

    // Standard Unlock operation
    void unlock()
    {
        int ret = pthread_mutex_unlock(&mutex_);
        if (ret != 0)
        {
            throw std::system_error(ret, std::generic_category(), "Failed to unlock mutex");
        }
    }

    // Direct access handle to the underlying POSIX object if required
    pthread_mutex_t* native_handle()
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
};
