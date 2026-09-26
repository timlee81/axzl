/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Thread.hpp"

#include "Error.hpp"

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetScheduler(const SchedulerParams& params)
{
    mSched = params;
    return *this;
}

/*
These individual scheduler options are probably undesirable
If you're going to set one... set both

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetSchedulerPolicy(SchedulerParams::Policy policy)
{
    // Or these individual Scheduler options
    if (!mSched)
        mSched = SchedulerParams { };

    mSched->policy = policy;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetSchedulerPriority(int prio)
{
    if (!mSched)
        mSched = SchedulerParams { };

    mSched->prio = prio;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetSchedulerNice(int nice)
{
    if (!mSched)
        mSched = SchedulerParams { };

    mSched->prio = nice;
    return *this;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetDetached(bool detached)
{
    mDetach = detached;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetStackSize(std::size_t size)
{
    mStackSize = size;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetAffinity(cpu_set_t cores)
{
    mCpuCores = cores;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Config& Thread::Config::SetAffinity(const std::initializer_list<unsigned int>& cores)
{
    cpu_set_t cset = { };
    for (auto& c : cores)
        CPU_SET(c, &cset);
    mCpuCores = cset;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::ScopedPosixAttr::ScopedPosixAttr(string_view name, LogPtr log)
{
    int rc = pthread_attr_init(&attr);
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_attr_init failure");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::ScopedPosixAttr::~ScopedPosixAttr()
{
    (void)pthread_attr_destroy(&attr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Attributes::Attributes(string_view name, LogPtr log, const Config& cfg)
: mAttr(name, log)
{
    // Either pull attributes from current thread OR use system defaults
    // Will override individual settings if specified by user
    if (cfg.mInherit)
    {
        int rc = pthread_getattr_np(pthread_self(), &mAttr.attr);
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_getattr_np");
    }
    else
    {
        int rc = pthread_getattr_default_np(&mAttr.attr);
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_getattr_default_np");
    }

    if (cfg.mSched)
    {
        // Validate settings
        if (cfg.mSched->IsRt())
        {
            // Validate RT prio
            int pmax = sched_get_priority_max(SCHED_RR);
            int pmin = sched_get_priority_min(SCHED_RR);
            if (cfg.mSched->prio > pmax || cfg.mSched->prio < pmin)
                ThrowSystemError(name, log, __func__, EINVAL, "RT priority out of range");
        }
        else
        {
            constexpr int LINUX_MIN_NICE = -20;
            constexpr int LINUX_MAX_NICE = 19;
            if (cfg.mSched->prio < LINUX_MIN_NICE || cfg.mSched->prio > LINUX_MAX_NICE)
                ThrowSystemError(name, log, __func__, EINVAL, "Nice out of range");
        }

        //  This is required to deviate from parent scheduling parameters
        int rc = pthread_attr_setinheritsched(&mAttr.attr, PTHREAD_EXPLICIT_SCHED);
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setinheritsched");

        // Get scheduler policy
        // int policy;
        // rc = pthread_attr_getschedpolicy(&mAttr.attr, &policy);
        // if (rc != 0)
        //    ThrowSystemError(name, log, __func__, rc, "pthread_attr_getschedpolicy");

        // if (cfg.mSched->policy)
        //{
        // rc = pthread_attr_setschedpolicy(&mAttr.attr, static_cast<int>(*cfg.mSched->policy));
        // if (rc != 0)
        //     ThrowSystemError(name, log, __func__, rc, "pthread_attr_setschedpolicy");
        //}
        rc = pthread_attr_setschedpolicy(&mAttr.attr, static_cast<int>(cfg.mSched->policy));
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setschedpolicy");

        // If RT, set prio.
        if (cfg.mSched->IsRt())
        {
            sched_param sp;
            sp.sched_priority = static_cast<int>(cfg.mSched->prio);

            rc = pthread_attr_setschedparam(&mAttr.attr, &sp);
            if (rc != 0)
                ThrowSystemError(name, log, __func__, rc, "pthread_attr_setschedparam");
        }

        // If SCHED_OTHER, set nice after thread has started
    }

    if (cfg.mDetach)
    {
        int detach = cfg.mDetach.value() ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
        int rc = pthread_attr_setdetachstate(&mAttr.attr, detach);
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setdetachstate");
    }

    if (cfg.mStackSize)
    {
        if (*cfg.mStackSize < PTHREAD_STACK_MIN)
            ThrowSystemError(name, log, __func__, ENOMEM, "mStackSize");

        int rc = pthread_attr_setstacksize(&mAttr.attr, cfg.mStackSize.value());
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setstack");
    }

    if (cfg.mGuardSize)
    {
        int rc = pthread_attr_setguardsize(&mAttr.attr, cfg.mGuardSize.value());
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setguardsize");
    }

    if (cfg.mCpuCores)
    {
        int rc = pthread_attr_setaffinity_np(&mAttr.attr, sizeof(cpu_set_t), &cfg.mCpuCores.value());
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_attr_setaffinity_np");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread::Thread(Thread&& from)
: mName(std::move(from.mName))
, mLog(from.mLog)
, mJoin(from.mJoin)
, mTid(from.mTid)
{
    /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
    from.mJoin = false;
    from.mLog = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Thread& Thread::operator=(Thread&& from)
{
    if (this != &from)
    {
        if (mJoin)
            Join();

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

///////////////////////////////////////////////////////////////////////////////////////////////////
void Thread::Join()
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

///////////////////////////////////////////////////////////////////////////////////////////////////
void Thread::PostThreadEntryInit()
{
    // If non-realtime, set nice value
    if (mSched && !mSched->IsRt())
    {
        int rc = setpriority(PRIO_PROCESS, 0, mSched->prio);
        if (rc != 0)
            ThrowSystemError(mName, mLog, __func__, rc, "setpriority");
    }

    // Set thread name, remember Linux only uses 15 characters
    constexpr std::size_t MAX_THREADNAME_CHARS = 15;
    pthread_setname_np(pthread_self(),
        mName.substr(0, MAX_THREADNAME_CHARS).c_str());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Thread::UpdatePriority(int prio)
{
    // Don't reference mSched... just ask OS

    // Get scheduler policy
    int policy;
    struct sched_param sp;
    int rc = pthread_getschedparam(mTid, &policy, &sp);
    if (rc != 0)
        ThrowSystemError(mName, mLog, __func__, rc, "pthread_getschedparam");

    // If RT, update via
    SchedulerParams sps { static_cast<SchedulerParams::Policy>(policy), sp.sched_priority };
    if (sps.IsRt())
    {
        rc = pthread_setschedprio(mTid, prio);
        if (rc != 0)
            ThrowSystemError(mName, mLog, __func__, rc, "pthread_setschedprio");
    }
    else
    {
        int rc = setpriority(PRIO_PROCESS, 0, prio);
        if (rc != 0)
            ThrowSystemError(mName, mLog, __func__, rc, "setpriority");
    }
    // Update mSched just in-case
    if (mSched)
        mSched->prio = prio;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Thread::Detach()
{
    if (mJoin)
    {
        int rc = pthread_detach(mTid);
        if (rc != 0)
            ThrowSystemError(mName, mLog, __func__, rc, "pthread_detach");

        mJoin = false;
    }
}

}
