/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Mutex.hpp"

#include "Error.hpp"

#include <pthread.h>

namespace Axzl
{
///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetRecursive()
{
    mType = PTHREAD_MUTEX_RECURSIVE;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetSharedMem()
{
    mShare = PTHREAD_PROCESS_SHARED;
    mRobust = PTHREAD_MUTEX_ROBUST;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetSharedMemDestroy()
{
    mShare = PTHREAD_PROCESS_SHARED;
    mRobust = PTHREAD_MUTEX_ROBUST;
    mSharedMemDestroy = true;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetRobust()
{
    mRobust = PTHREAD_MUTEX_ROBUST;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetPrioInherit()
{
    mProto = PTHREAD_PRIO_INHERIT;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Config& Mutex::Config::SetErrorCheck()
{
    mType = PTHREAD_MUTEX_ERRORCHECK;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::ScopedPosixAttr::ScopedPosixAttr(string_view name, LogPtr log)
{
    int rc = pthread_mutexattr_init(&attr);
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_init failure");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::ScopedPosixAttr::~ScopedPosixAttr()
{
    (void)pthread_mutexattr_destroy(&attr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
Mutex::Attributes::Attributes(string_view name, LogPtr log, const Config& cfg)
: mAttr(name, log)
{
    int rc = pthread_mutexattr_settype(&mAttr.attr, static_cast<int>(cfg.mType));
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_settype");

    rc = pthread_mutexattr_setpshared(&mAttr.attr, static_cast<int>(cfg.mShare));
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_setpshared");

    rc = pthread_mutexattr_setrobust(&mAttr.attr, static_cast<int>(cfg.mRobust));
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_setrobust");

    rc = pthread_mutexattr_setprotocol(&mAttr.attr, static_cast<int>(cfg.mProto));
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_setprotocol");

    if (cfg.mProto == PTHREAD_PRIO_PROTECT)
    {
        rc = pthread_mutexattr_setprioceiling(&mAttr.attr, cfg.mPrioCeiling);
        if (rc != 0)
            ThrowSystemError(name, log, __func__, rc, "pthread_mutexattr_setprioceiling");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::Init(const Mutex::Config& cfg)
{
    // Scoped attribute will automatically clean-up via ScopedAttr
    Attributes attr { mName, mLog, cfg };

    int rc = pthread_mutex_init(mMutex, &attr.mAttr.attr);
    if (rc != 0)
    {
        ThrowSystemError(mName, mLog, __func__, rc, "pthread_mutex_init");
    }
    else
    {
        mValid = true;
        mRobust = (cfg.mRobust == PTHREAD_MUTEX_ROBUST);
        mShared = (cfg.mShare == PTHREAD_PROCESS_SHARED);
        mSharedCleanup = cfg.mSharedMemDestroy;

        mLog->Debug("{}: Mutex init succeeded on '{}'", __func__, mName);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::LockFail(int rc)
{
    if (rc == EOWNERDEAD && mRobust)
    {
        // Previous owner died (EOWNERDEAD) — mutex is locked but marked inconsistent.
        // Only one thread waiting on pthread_mutex_lock() is given EOWNERDEAD
        // Repair is the caller's responsibility before the next Unlock();
        //  we make it consistent internally so the mutex remains usable.
        int rcConsistent = pthread_mutex_consistent(mMutex);
        if (rcConsistent == 0)
        {
            mLog->Info("{}: Mutex consistency restored on '{}'", __func__, mName);
        }
        else
        {
            ThrowSystemError(mName, mLog, __func__, rc, "pthread_mutex_consistent failure");
        }
    }
    else
    {
        ThrowSystemError(mName, mLog, __func__, rc, "pthread_mutex_lock");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::UnlockFail(int rc)
{
    ThrowSystemError(mName, mLog, __func__, rc, "pthread_mutex_unlock");
}
}
