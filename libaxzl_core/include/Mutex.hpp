/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.h"

#include <pthread.h>
#include <string>
#include <system_error>

namespace Axzl
{

/**
 * Mutex Attribute values
 */

enum class MutexType
{
    Normal = PTHREAD_MUTEX_NORMAL,
    ErrorCheck = PTHREAD_MUTEX_ERRORCHECK,
    Recursive = PTHREAD_MUTEX_RECURSIVE,
    // Avoid this type - vague
    Default = PTHREAD_MUTEX_DEFAULT,
};

enum class MutexShared
{
    Private = PTHREAD_PROCESS_PRIVATE,
    Shared = PTHREAD_PROCESS_SHARED,
};

enum class MutexRobust
{
    Stalled = PTHREAD_MUTEX_STALLED,
    Robust = PTHREAD_MUTEX_ROBUST,
};

enum class MutexProtocol
{
    None = PTHREAD_PRIO_NONE,
    Inherit = PTHREAD_PRIO_INHERIT,
    Protect = PTHREAD_PRIO_PROTECT,
};

/**
 * Attribute Object
 */
struct MutexAttributes
{
    MutexType type = MutexType::Normal;
    MutexShared shared = MutexShared::Private;
    MutexRobust robust = MutexRobust::Stalled;
    MutexProtocol proto = MutexProtocol::None;
    int priorityCeiling = 0;
};

/**
 * Common Mutex Types
 */

// Standard thread synchronization within a single process.
inline constexpr MutexAttributes DefaultConfig { };

// Allows a single thread to safely relock the same mutex recursively.
inline constexpr MutexAttributes RecursiveConfig { MutexType::Recursive };

// Safe debugging profile that returns an error code on deadlocks or improper unlocks.
inline constexpr MutexAttributes ErrorCheckConfig { MutexType::ErrorCheck };

// Inter-process communication profile (for use in shared memory).
inline constexpr MutexAttributes SharedConfig { MutexType::Normal, MutexShared::Shared };

// Inter-process communication that safely reports if a process crashes while holding the lock.
inline constexpr MutexAttributes RobustSharedConfig { MutexType::Normal, MutexShared::Shared, MutexRobust::Robust };

/**
 * Mutex type for real POSIX systems
 * std::mutex is lovely, but it lacks attributes needed for constrained systems.
 *
 * Implement Lockable
 *  Also should implement TryLockable and TimedLockable
 */
class Mutex
{
public:
    /**
     * Simple constructor — takes just a name and uses defaults for log and attributes
     *
     * @param name Name of the mutex for debug
     * @param log Logger Log to use for errors, or the default log.
     * @param attrs POSIX Mutex attributes
     */
    explicit Mutex(const char* name = "NotSmartMtx",
        std::shared_ptr<Log> log = std::shared_ptr<Log> { nullptr },
        /* add me instead */
        /*std::shared_ptr<Log> log = std::shared_ptr<Log> { GetLog() },*/
        const MutexAttributes& attrs = DefaultConfig)
    : mName(name)
    , mLog(log)
    , mMutexAttrs(attrs)
    {
        if (!mLog)
            // SAME AS ABOVE DEFAULT
            //            mLog = std::make_shared<
            // GET global logger here
            ;
        Init();
    }

    /** Move constructor  */
    Mutex(Mutex&& other) = delete;
    /*
    Mutex(Mutex&& other) noexcept
    : mMutex(other.mMutex)
    , mRobust(other.mRobust)
    , mValid(other.mValid)
    {
        other.mValid = false;
    }
    */

    /** Move-assign omitted: would need to destroy an existing mutex first. */
    Mutex& operator=(Mutex&&) = delete;

    /** Destructor */
    ~Mutex() noexcept
    {
        if (mValid)
            pthread_mutex_destroy(&mMutex);
    }

    /** Disable Copy and Assignment */
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void Lock()
    {
        int rv = pthread_mutex_lock(&mMutex);
        if (rv != 0)
            LockFail(rv);
    }
    /** Lockable compliant */
    void lock() { Lock(); }

    void Unlock()
    {
        int rv = pthread_mutex_unlock(&mMutex);
        if (rv != 0)
            UnlockFail(rv);
    }
    /** Lockable compliant */
    void unlock() { Unlock(); }

    /**
     *  Returns true if the lock was acquired, false if already held (EBUSY).
     * noexcept per standard Lockable convention; non-zero result → false.
     */
    bool TryLock() noexcept
    {
        return pthread_mutex_trylock(&mMutex) == 0;
    }
    /** Lockable compliant */
    void try_lock() { TryLock(); }

    // pthread_mutex_t* Handle() noexcept { return &mMutex; }
    // const pthread_mutex_t* Handle() const noexcept { return &mMutex; }

private:
    /**
     * Initialize the mutex with attributes
     */
    void Init();

    /** Long-form for failure cases */
    void LockFail(int rv);
    void UnlockFail(int rv);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    /** Mutex name */
    std::string mName;

    /** Cached mutex attributes */
    MutexAttributes mMutexAttrs;

    /** Log interface */
    std::shared_ptr<Log> mLog;

    /** Mutex */
    pthread_mutex_t mMutex { };

    /** Valid flag */
    bool mValid { false };

    /** Robust flag */
    bool mRobust { false };
};
}
