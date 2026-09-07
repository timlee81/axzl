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
#include <memory>
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
     * Constructor
     *  Create and run a new thread
     */
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        /* Thread Parameters */
        const SchedulerParams& sched,
        Func&& func, Args&&... args)
    : mName(name.empty() ? "NotSmartThread" : name)
    , mLog(log ? log : GetLog())
    , mSchedParams(sched)
    {
        /// todo
        // Validate scheduler
        // static_assert...

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
        auto attr = CreateAttributes();

        // Start thread
        int rc = pthread_create(&mTid, attr.get(), &Thread::ThreadEntry<CallData>, ctx.get());
        if (rc != 0)
        {
            // Throw
            ThrowSystemError(mLog, mName, __func__, rc, "pthread_create");
        }
        else
        {
            /* New thread created, release callData to thread */
            ctx.release();
            mJoin = true;
            mLog->Trace("{}:{} thread ctor", __func__, mName);
        }
    }
#if 0
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        Func&& func, Args&&... args)
    : Thread(log, {}, std::forward<Func>(func), std::forward<Args>(args)...)
    {
    }
#endif

    /** Join thread - block waiting for join to complete */
    void Join()
    {
        if (mJoin)
        {
            // Do join
            mLog->Debug("{}:{} begin join...", __func__, mName);
            void* threadRet;
            int rc = pthread_join(mTid, &threadRet);
            if (rc != 0)
                mLog->Error("{}:{} join error '{}'", __func__, mName, strerror(rc));
            else
                mLog->Debug("{}:{} joined", __func__, mName);

            mJoin = false;
        }
    }

    /** Destructor */
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
    , mSchedParams(from.mSchedParams)
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
            mSchedParams = from.mSchedParams;
            /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
            from.mJoin = false;
            from.mLog = nullptr;
        }
        return *this;
    }

private:
    struct AttributeDestroyer
    {
        void operator()(pthread_attr_t* attr) const
        {
            pthread_attr_destroy(attr);
            delete attr;
        }
    };
    using Attributes = std::unique_ptr<pthread_attr_t, AttributeDestroyer>;

    Attributes CreateAttributes()
    {
        auto attr = Attributes(new pthread_attr_t);

        int rc = pthread_attr_init(attr.get());
        if (rc != 0)
            ThrowSystemError(mLog, mName, __func__, rc, "pthread_attr_init");

        return attr;
    }

    /**
     * Setup thread after ThreadEntry
     */
    void PostThreadEntryInit()
    {
        // can you only set nice after thread create?

        /** If non-realtime, set nice value */
        if (mSchedParams.policy != SchedulerParams::Policy::RealtimeFifo
            && mSchedParams.policy != SchedulerParams::Policy::RealtimeRoundRobin
            && mSchedParams.policy != SchedulerParams::Policy::RealtimeDeadline)
        {
            // setpriority(PRIO_PROCESS, 0, This->mSchedParams.prio);
        }

        // nice value??

        // Set thread name, remember Linux only uses 15 characters
        constexpr std::size_t MAX_THREADNAME_CHARS = 15;
        pthread_setname_np(pthread_self(),
            mName.substr(MAX_THREADNAME_CHARS).c_str());
    }

    /**
     * Context to pass to thread
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

    /** Scheduler data */
    SchedulerParams mSchedParams;
};
}
