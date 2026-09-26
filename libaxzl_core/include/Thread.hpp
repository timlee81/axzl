/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Error.hpp"
#include "Log.hpp"
#include "Scheduler.hpp"
#include "StringView.hpp"

#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <pthread.h>
#include <string>
#include <sys/resource.h>
#include <tuple>
#include <utility>

namespace Axzl
{

/**
 * Thread
 *
 * Maintain std::thread semantics, but allow pthread attribute setting.
 */
class Thread
{
public:
    /**
     * Configuration
     */
    struct Config
    {
        bool mInherit;
        std::optional<SchedulerParams> mSched = std::nullopt;
        std::optional<bool> mDetach = std::nullopt;
        std::optional<std::size_t> mStackSize = std::nullopt;
        std::optional<std::size_t> mGuardSize = std::nullopt;
        std::optional<cpu_set_t> mCpuCores = std::nullopt;

        /**
         *  Create Config
         *  Either inherit from current thread, or use system defaults.
         *  Scheduler parameters are _always_ inherited from calling thread, unless
         *      explicitly defined.
         */
        explicit Config(bool inherit = true)
        : mInherit(inherit)
        {
        }

        // Combination of all scheduler parameters
        Config& SetScheduler(const SchedulerParams& params);
        // Or these individual Scheduler options
        /* These are probably undesirable... set both, not one at a time
        Config& SetSchedulerPolicy(SchedulerParams::Policy policy);
        Config& SetSchedulerPriority(int prio);
        Config& SetSchedulerNice(int nice);
        */

        // Other parameters
        Config& SetDetached(bool detached);
        Config& SetStackSize(std::size_t size);
        Config& SetAffinity(cpu_set_t cores);
        Config& SetAffinity(const std::initializer_list<unsigned int>& cores);
    };

    /**
     * Constructor
     *  Create and run a new thread
     */
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        const Config& cfg,
        Func&& func, Args&&... args)
    : mName(name.empty() ? "NotSmartThread" : name)
    , mLog(log ? log : GetLog())
    {
        /**
         * Bundle the callable and arguments into a tuple. Decay ensures we copy values
         *  rather than storing dangling refs to variables on the caller's stack.
         * Heap because it must outlive this call; the new thread may start running
         *  well after the constructor returns.
         */
        using CallData = std::tuple<std::decay_t<Func>, std::decay_t<Args>...>;
        auto ctx = std::make_unique<Context<CallData>>(
            this, CallData(std::forward<Func>(func), std::forward<Args>(args)...));

        // Get attributes
        Attributes attr { mName, mLog, cfg };
        // Save sched prior to launching new thread - need to know if setting nice value
        mSched = cfg.mSched;

        // Start thread
        int rc = pthread_create(&mTid, &attr.mAttr.attr, &Thread::ThreadEntry<CallData>, ctx.get());
        if (rc != 0)
        {
            // Throw
            ThrowSystemError(mName, mLog, __func__, rc, "pthread_create");
        }
        else
        {
            /* New thread created, release callData to thread */
            ctx.release();
            if (cfg.mDetach && cfg.mDetach.value())
                mJoin = false;
            else
                mJoin = true;

            mLog->Trace("{}:{} thread ctor (detached:{})", __func__, mName, mJoin);
        }
    }

    /**
     * Create a new Thread
     * Inherit current thread
     */
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        Func&& func, Args&&... args)
    : Thread(name, log, Config { true }, std::forward<Func>(func), std::forward<Args>(args)...)
    {
    }

    /**
     * Create a new Thread
     * Inherit current thread, but set an explicit scheduler policy / parameters
     */
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        const SchedulerParams& sched,
        Func&& func, Args&&... args)
    : Thread(log, Config { true }.SetScheduler(sched), std::forward<Func>(func), std::forward<Args>(args)...)
    {
    }

    /** Destructor - will Join() by default */
    ~Thread()
    {
        Join();
        // mLog may have been moved, test
        if (mLog)
            mLog->Trace("{}:{} thread dtor", __func__, mName);
    }

    /** Not copyable */
    Thread(Thread& copy) = delete;
    Thread& operator=(Thread& assign) = delete;

    /** Moving is supported, pthread_t can be copied but not compared without pthread_equal */
    Thread(Thread&& from)
    : mName(std::move(from.mName))
    , mLog(from.mLog)
    , mJoin(from.mJoin)
    , mTid(from.mTid)
    {
        /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
        from.mJoin = false;
        from.mLog = nullptr;
    }

    Thread& operator=(Thread&& from)
    {
        if (this != &from)
        {
            mName = std::move(from.mName);
            mLog = from.mLog;
            mJoin = from.mJoin;
            mTid = from.mTid;
            /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
            from.mJoin = false;
            from.mLog = nullptr;
        }
        return *this;
    }

    /** Join thread - block waiting for join to complete */
    void Join();

    /**
     * Return native handle for OS operations, if desired
     */
    using NativeHandleType = pthread_t;
    NativeHandleType NativeHandle() { return mTid; }
    using native_handle_type = pthread_t;
    native_handle_type native_handle() { return NativeHandle(); }

    /**
     * Update the RT priority or nice value after the thread has started
     *
     * @prio RT Prio or nice value to set
     */
    void UpdatePriority(int prio);

private:
    /**
     * Scoped wrapper around pthread_attr_t
     *  _attr_init is called on construction / _destroy on destruction
     */
    struct ScopedPosixAttr
    {
        // pthread_mutexattr_t is not copyable/movable
        pthread_attr_t attr;

        ScopedPosixAttr(string_view name, LogPtr log);
        ~ScopedPosixAttr();
    };

    /** POSIX Thread Attributes for thread */
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
     * Setup thread after ThreadEntry
     */
    void PostThreadEntryInit();

    /**
     * Context to pass to thread - used to pass both 'this', and CallData (function & args)
     */
    template <typename CallData>
    struct Context
    {
        Thread* This;
        CallData callData;

        Context(Thread* t, CallData c)
        : This(t)
        , callData(std::move(c))
        {
        }
    };

    /**
     * Thread Entry point - to avoid type-erasure, use a (small) template
     */
    template <typename CallData>
    static void* ThreadEntry(void* arg)
    {
        /* Grab This, function and arguments */
        std::unique_ptr<Context<CallData>> ctx(static_cast<Context<CallData>*>(arg));
        Thread* This = ctx->This;

        // Setup thread
        This->PostThreadEntryInit();

        /* apply calls the lambda with the tuple args unpacked */
        std::apply(
            [](auto&&... unpacked)
            {
                /* invoke will handle the difference between standard function pointers
                    and member function pointers, else could have made the lambda
                    be [](auto&& f, auto&&... unpacked) { f((unpacked...));} */
                std::invoke(std::forward<decltype(unpacked)>(unpacked)...);
            },
            std::move(ctx->callData));
        return nullptr;
    }

    /** Thread name - set in OS to be viewed by 'ps' */
    std::string mName;

    /** Logger */
    LogPtr mLog;

    /** Is the thread joinable? */
    bool mJoin { false };

    /** Thread ID */
    pthread_t mTid;

    /** If defined, scheduling parameters */
    std::optional<SchedulerParams> mSched { std::nullopt };
};

}
