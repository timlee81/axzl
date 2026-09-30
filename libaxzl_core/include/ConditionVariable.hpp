/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "ClockUtility.hpp"
#include "Log.hpp"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <pthread.h>
#include <string>
#include <system_error>
#include <time.h>
#include <type_traits>

namespace Axzl
{

/**
 * Condition Variable type for real POSIX systems
 * This is a std::condition_variable_any- like class (not std::condition_variable)
 * std::condition_variable_any is lovely, but it lacks attributes needed for constrained systems.
 *
 */
class ConditionVariable
{
public:
    enum class CvStatus
    {
        Timeout,
        NoTimeout,
    };

    /**
     * Configuration
     */
    struct Config
    {
        int mShare { PTHREAD_PROCESS_PRIVATE };
        bool mSharedMemDestroy { false };

        /** SHARED, NO Destroy on destruction */
        Config& SetSharedMem();
        /** SHARED, _Destroy_ on destruction */
        Config& SetSharedMemDestroy();
    };

    /** No default constructor */
    ConditionVariable() = delete;

    /**
     * Simple constructor — takes just a name and uses defaults for log and attributes
     *
     * @param name Name of the condition variable for debug
     * @param log Logger Log to use for errors, or the default log.
     * @param attrs POSIX Condition Variable attributes
     */
    explicit ConditionVariable(string_view name,
        LogPtr log,
        const Config& cfg)
    : mName(name.empty() ? "NotSmartCv" : name)
    , mLog(log ? std::move(log) : GetLog())
    {
        mCv = new pthread_cond_t;
        Init(cfg);
    }
    /** Call ConditionVariable() with no Config, work around gcc/clang bug */
    explicit ConditionVariable(string_view name, LogPtr log)
    : ConditionVariable(name, log, Config { })
    {
    }

    /** Shared memory constructor */
    explicit ConditionVariable(string_view name,
        LogPtr log,
        void* cv,
        bool create,
        const Config& cfg)
    : mName(name.empty() ? "NotSmartCv" : name)
    , mLog(log ? std::move(log) : GetLog())
    {
        mCv = reinterpret_cast<pthread_cond_t*>(cv);
        if (create)
            Init(cfg);
    }
    /** Call ConditionVariable() with no Config, work around gcc/clang bug */
    explicit ConditionVariable(string_view name, LogPtr log, void* cv, bool create)
    : ConditionVariable(name, log, cv, create, Config { }.SetSharedMem())
    {
    }

    /** Short-hand factories */
    static ConditionVariable Make(string_view name, LogPtr log)
    {
        return ConditionVariable(name, log);
    }
    static ConditionVariable CreateShm(string_view name, LogPtr log, void* cv, bool destroy = false)
    {
        return ConditionVariable(name, log, cv, true,
            destroy ? Config { }.SetSharedMemDestroy() : Config { }.SetSharedMem());
    }
    static ConditionVariable OpenShm(string_view name, LogPtr log, void* cv, bool destroy = false)
    {
        return ConditionVariable(name, log, cv, false);
    }

    /** Move constructor  */
    ConditionVariable(ConditionVariable&& from) = delete;
    /** Move-assign omitted: would need to destroy an existing cv first. */
    ConditionVariable& operator=(ConditionVariable&& from) = delete;

    /** Disable Copy and Assignment */
    ConditionVariable(const ConditionVariable& from) = delete;
    ConditionVariable& operator=(const ConditionVariable& from) = delete;

    /** Destructor */
    ~ConditionVariable() noexcept
    {
        if (mValid)
        {
            if (!mShared || (mShared && mSharedCleanup))
                pthread_cond_destroy(mCv);
        }

        if (!mShared && mCv)
            delete mCv;

        mCv = nullptr;
    }

    void NotifyOne() noexcept
    {
        pthread_cond_signal(mCv);
    }
    void notify_one() noexcept { NotifyOne(); }

    void NotifyAll() noexcept
    {
        pthread_cond_broadcast(mCv);
    }
    void notify_all() noexcept { NotifyAll(); }

    //
    template <typename Lock>
    void Wait(Lock& lock)
    {
        int rc = pthread_cond_wait(&mCv, lock.mutex());
        // There's not much to actually check, unlike robust mutex... just throw
        if (rc != 0)
            ThrowSystemError(mName, mLog, __func__, rc, "pthread_cond_wait");
    }

    template <typename Lock>
    void wait(Lock& lock)
    {
        Wait(lock);
    }

    template <typename Lock, typename Predicate>
    void Wait(Lock& lock, Predicate pred)
    {
        while (!pred())
            Wait(lock);
    }

    template <typename Lock, typename Predicate>
    void wait(Lock& lock, Predicate pred)
    {
        Wait(lock, std::move(pred));
    }

    // duration
    template <typename Lock, typename Rep, typename Period>
    CvStatus WaitFor(Lock& lock, const std::chrono::duration<Rep, Period>& duration)
    {
        //  Convert to WaitUntil
        auto timeout = std::chrono::steady_clock::now() + duration;
        return WaitUntil(lock, timeout);
    }

    template <typename Lock, typename Rep, typename Period, typename Predicate>
    bool WaitFor(Lock& lock, const std::chrono::duration<Rep, Period>& duration, Predicate pred)
    {
        //  Convert to WaitUntil
        auto timeout = std::chrono::steady_clock::now() + duration;
        return WaitUntil(lock, timeout, std::move(pred));
    }

    // time_point
    template <typename Lock, typename Clock, typename Duration>
    CvStatus WaitUntil(Lock& lock, const std::chrono::time_point<Clock, Duration>& timeoutTp)
    {
        //  Lookup clock_t
        clockid_t clkId = ClockId(timeoutTp);
        //  Convert timeout to posix abs time
        timespec ts = DurationToTimespec(timeoutTp);

        // Wait on the requested clock
        int rc = pthread_cond_clockwait(mCv, lock.mutex(), clkId, &ts);
        if (rc == 0)
            return CvStatus::NoTimeout;
        else if (rc == ETIMEDOUT)
            return CvStatus::Timeout;
        else
            ThrowSystemError(mName, mLog, __func__, rc, "WaitUntil pthread_cond_clockwait");
    }

    // time_point
    template <typename Lock, typename Clock, typename Duration, typename Predicate>
    bool WaitUntil(Lock& lock, const std::chrono::time_point<Clock, Duration>& timeoutTp, Predicate pred)
    {
        //  Lookup clock_t
        clockid_t clkId = ClockId(timeoutTp);
        //  Convert timeout to posix abs time
        timespec ts = DurationToTimespec(timeoutTp);

        // Wait until predicate is true or timeout
        bool predReturn;
        while ((predReturn = pred()) == false)
        {
            // Wait on the requested clock
            int rc = pthread_cond_clockwait(mCv, lock.mutex(), clkId, &ts);
            if (rc == 0)
                continue;
            else if (rc == ETIMEDOUT)
                break;
            else
                ThrowSystemError(mName, mLog, __func__, rc, "WaitUntil pthread_cond_clockwait");
        }

        return predReturn;
    }

private:
    /** Scoped attribute for RAII / cleanup */
    struct ScopedPosixAttr
    {
        // pthread_condattr_t is not copyable/movable
        pthread_condattr_t attr;

        ScopedPosixAttr(string_view name, LogPtr log);
        ~ScopedPosixAttr();
    };

    struct Attributes
    {
        ScopedPosixAttr mAttr;

        Attributes(string_view name, LogPtr log, const Config& cfg);

        Attributes(const Attributes& from) = delete;
        Attributes(Attributes&& from) = delete;
        Attributes& operator=(const Attributes& from) = delete;
        Attributes& operator=(Attributes&& from) = delete;
    };

    /**
     * Initialize the condvar with attributes
     */
    void Init(const Config& cfg);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    /** Condvar name */
    std::string mName;

    /** Log interface */
    LogPtr mLog;

    /** Condvar */
    pthread_cond_t* mCv { nullptr };

    /** Valid flag */
    bool mValid { false };

    /** Shared flag */
    bool mShared { false };
    bool mSharedCleanup { false };
};
}
