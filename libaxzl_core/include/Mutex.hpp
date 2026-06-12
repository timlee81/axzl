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
template <
    MutexType Type = MutexType::Normal,
    MutexShared Shared = MutexShared::Private,
    MutexRobust Robust = MutexRobust::Stalled,
    MutexProtocol Proto = MutexProtocol::None,
    int Ceiling = 0>
struct MutexAttr
{
    static constexpr MutexType type = Type;
    static constexpr MutexShared shared = Shared;
    static constexpr MutexRobust robust = Robust;
    static constexpr MutexProtocol proto = Proto;
    static constexpr int priorityCeiling = Ceiling;
};

/**
 * Common Mutex Types
 */

// Standard thread synchronization within a single process.
using DefaultConfig = MutexAttr<>;

// Allows a single thread to safely relock the same mutex recursively.
using RecursiveConfig = MutexAttr<MutexType::Recursive>;

// Safe debugging attribute that returns an error code on deadlocks or improper unlocks.
using ErrorCheckConfig = MutexAttr<MutexType::ErrorCheck>;

// Inter-process communication attribute (for use in shared memory).
using SharedConfig = MutexAttr<MutexType::Normal, MutexShared::Shared>;

// Inter-process communication that safely reports if a process crashes while holding the lock.
using RobustSharedConfig = MutexAttr<MutexType::Normal, MutexShared::Shared, MutexRobust::Robust>;

/**
 * Helper for failed construction
 */
//[[noreturn]] inline void DestroyAttrAndThrow(pthread_mutexattr_t& attr, int rv, const char* what)
//{
//    pthread_mutexattr_destroy(&attr);
//    throw std::system_error(rv, std::system_category(), what);
//}

/**
 * Implement Lockable
 *
 *    Also should implement TryLockable and TimedLockable
 */
class Mutex
{
public:
    /**
     * Simple constructor — takes just a name and uses defaults for log and attributes
     */
    explicit Mutex(const char* name = "NotSmartMtx",
        std::shared_ptr<Log> log = std::shared_ptr<Log> { nullptr })
    : mName(name)
    , mLog(std::shared_ptr<Log>(nullptr))
    {
        // Init(DefaultConfig { });
        Init<DefaultConfig>();
    }

    /**
     *  Templated constructor maps a MutexAttr's static fields to pthread
     * attribute calls at compile time. All setter return values are checked;
     * any failure destroys the attr and throws before touching mMutex.
     */
    template <typename Attrib>
    explicit Mutex(const char* name = "NotSmartMtx",
        std::shared_ptr<Log> log = std::shared_ptr<Log> { nullptr },
        const Attrib& attr = DefaultConfig { })
    : mName(name)
    , mLog(log)
    {
        Init<Attrib>();
    }

    /**
     * Default constructor
     */
    /*
    Mutex()
    : Mutex("Not Smart", nullptr, DefaultConfig { })
    {
    }
    */

    /**
     * Move constructor — invalidates the source to prevent double-destroy.
     */
    Mutex(Mutex&& other) noexcept
    : mMutex(other.mMutex)
    , mValid(other.mValid)
    {
        other.mValid = false;
    }

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

    // Returns true if the lock was acquired, false if already held (EBUSY).
    // noexcept per standard Lockable convention; non-zero result → false.
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
     *  Templated constructor maps a MutexAttr's static fields to pthread
     * attribute calls at compile time. All setter return values are checked;
     * any failure destroys the attr and throws before touching mMutex.
     */
    template <typename Attrib>
    // void Init(Attrib&& /*tag*/)
    void Init(/*tag*/)
    {
        pthread_mutexattr_t attr;
        int rv;
        try
        {
            rv = pthread_mutexattr_init(&attr);
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutexattr_init");

            rv = pthread_mutexattr_settype(&attr, static_cast<int>(Attrib::type));
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutexattr_settype");

            rv = pthread_mutexattr_setpshared(&attr, static_cast<int>(Attrib::shared));
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutexattr_setpshared");

            rv = pthread_mutexattr_setrobust(&attr, static_cast<int>(Attrib::robust));
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutexattr_setrobust");

            rv = pthread_mutexattr_setprotocol(&attr, static_cast<int>(Attrib::proto));
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutexattr_setprotocol");

            if constexpr (Attrib::proto == MutexProtocol::Protect)
            {
                rv = pthread_mutexattr_setprioceiling(&attr, Attrib::priorityCeiling);
                if (rv != 0)
                    throw std::system_error(rv, std::system_category(), "pthread_mutexattr_setprioceiling");
            }

            rv = pthread_mutex_init(&mMutex, &attr);
            if (rv != 0)
                throw std::system_error(rv, std::system_category(), "pthread_mutex_init");

            mValid = true;
            pthread_mutexattr_destroy(&attr);
        }
        // REVISIT
        catch (...)
        {
            pthread_mutexattr_destroy(&attr);
        }
    }

    /**
     * Call after catching EOWNERDEAD on a robust mutex to signal that shared
     * state has been repaired and the mutex is usable again.
     *
     * Typical usage:
     * try { m.Lock(); }
     * catch (const std::system_error& e) {
     *     if (e.code().value() == EOWNERDEAD) { Repair(); m.Consistent(); }
     *     else throw;
     * }
     */
    bool Consistent()
    {
        int rv = pthread_mutex_consistent(&mMutex);
        if (rv != 0)
            return false;
        return true;
    }

    void LockFail(int rv)
    {
        //            if (mLog)
        // throw std::system_error(rv, std::system_category(), "pthread_mutex_lock");
    }

    void UnlockFail(int rv)
    {
        //            if (mLog)
        // throw std::system_error(rv, std::system_category(), "pthread_mutex_lock");
    }
    /** Mutex name */
    std::string mName;

    /** Log interface */
    std::shared_ptr<Log> mLog;

    /** Mutex */
    pthread_mutex_t mMutex { };

    /** Valid flag */
    bool mValid = false;
};

}
