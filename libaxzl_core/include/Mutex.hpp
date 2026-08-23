/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"

#include <memory>
#include <pthread.h>
#include <string>
#include <system_error>

namespace Axzl
{

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
     * Configuration
     */
    struct Config
    {
        int mType { PTHREAD_MUTEX_NORMAL };
        int mShare { PTHREAD_PROCESS_PRIVATE };
        bool mSharedMemDestroy { false };
        int mRobust { PTHREAD_MUTEX_STALLED };
        int mProto { PTHREAD_PRIO_NONE };
        int mPrioCeiling { -1 };

        Config& SetRecursive();
        /** ROBUST and SHARED set, NO Destroy on destruction */
        Config& SetSharedMem();
        /** ROBUST and SHARED set, _Destroy_ on destruction */
        Config& SetSharedMemDestroy();
        /** This function is only for non shared ROBUST operation (not likely) */
        Config& SetRobust();
        /** If you want explicit PTHREAD_PRIO_PROTECT with ceiling, directly write to members  */
        Config& SetPrioInherit();
        /** Debug mode - can't be recursive at the same time */
        Config& SetErrorCheck();
    };

    Mutex() = delete;

    /**
     * Simple constructor — takes just a name and uses defaults for log and attributes
     *
     * @param name Name of the mutex for debug
     * @param log Logger Log to use for errors, or the default log.
     * @param attrs POSIX Mutex attributes
     */
    explicit Mutex(string_view name,
        LogPtr log,
        const Config& cfg)
    : mName(name.empty() ? "NotSmartMtx" : name)
    , mLog(std::move(log))
    {
        if (!mLog)
            mLog = GetLog();

        mMutex = new pthread_mutex_t;
        Init(cfg);
    }
    /** Call Mutex() with no Config, work around gcc/clang bug */
    explicit Mutex(string_view name, LogPtr log)
    : Mutex(name, log, Config { })
    {
    }

    /** Shared memory constructor */
    explicit Mutex(string_view name,
        LogPtr log,
        void* mtx,
        bool create,
        const Config& cfg)
    : mName(name.empty() ? "NotSmartMtx" : name)
    , mLog(std::move(log))
    {
        if (!mLog)
            mLog = GetLog();

        mMutex = reinterpret_cast<pthread_mutex_t*>(mtx);
        if (create)
            Init(cfg);
    }
    /** Call Mutex() with no Config, work around gcc/clang bug */
    explicit Mutex(string_view name, LogPtr log, void* mtx, bool create)
    : Mutex(name, log, mtx, create, Config { }.SetSharedMem())
    {
    }

    /** Short-hand factories */
    static Mutex Make(string_view name, LogPtr log)
    {
        return Mutex(name, log);
    }
    static Mutex MakeRecursive(string_view name, LogPtr log)
    {
        return Mutex(name, log, Config { }.SetRecursive());
    }
    // Should be robust and process shared
    static Mutex CreateShm(string_view name, LogPtr log, void* mtx, bool destroy = false)
    {
        return Mutex(name, log, mtx, true,
            destroy ? Config { }.SetSharedMemDestroy() : Config { }.SetSharedMem());
    }
    static Mutex OpenShm(string_view name, LogPtr log, void* mtx, bool destroy = false)
    {
        return Mutex(name, log, mtx, false);
    }

    /** Move constructor  */
    Mutex(Mutex&& other) = delete;
    /** Move-assign omitted: would need to destroy an existing mutex first. */
    Mutex& operator=(Mutex&&) = delete;

    /** Disable Copy and Assignment */
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    /** Destructor */
    ~Mutex() noexcept
    {
        if (mValid)
        {
            if (!mShared || (mShared && mSharedCleanup))
                pthread_mutex_destroy(mMutex);
        }

        if (!mShared && mMutex)
            delete mMutex;

        mMutex = nullptr;
    }

    /**
     * Lock Mutex
     *
     * Robust mutex lock failure will attempt make the mutex consistent and try again.
     */
    void Lock()
    {
        int rc = pthread_mutex_lock(mMutex);
        if (rc != 0)
            LockFail(rc);
    }
    /** Lockable compliant */
    void lock() { Lock(); }

    /**
     * Unlock Mutex
     */
    void Unlock()
    {
        int rc = pthread_mutex_unlock(mMutex);
        if (rc != 0)
            UnlockFail(rc);
    }
    /** Lockable compliant */
    void unlock() { Unlock(); }

    /**
     *  Returns true if the lock was acquired, false if already held (EBUSY).
     * noexcept per standard Lockable convention; non-zero result → false.
     */
    bool TryLock() noexcept
    {
        return pthread_mutex_trylock(mMutex) == 0;
    }
    /** Lockable compliant */
    void try_lock() { TryLock(); }

    // pthread_mutex_t* Handle() noexcept { return mMutex; }
    // const pthread_mutex_t* Handle() const noexcept { return mMutex; }

private:
    /**
     * Initialize the mutex with attributes
     */
    void Init(const Config& cfg);

    /** Long-form for failure cases */
    void LockFail(int rc);
    void UnlockFail(int rc);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    /** Mutex name */
    std::string mName;

    /** Log interface */
    LogPtr mLog;

    /** Mutex */
    pthread_mutex_t* mMutex { nullptr };

    /** Valid flag */
    bool mValid { false };

    /** Robust flag */
    bool mRobust { false };

    /** Shared flag */
    bool mShared { false };
    bool mSharedCleanup { false };
};
}
