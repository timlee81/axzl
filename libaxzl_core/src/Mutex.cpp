/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Mutex.hpp"

#include "Error.hpp"

#include <pthread.h>

namespace Axzl
{

Mutex::Config& Mutex::Config::SetRecursive()
{
    mType = PTHREAD_MUTEX_RECURSIVE;
    return *this;
}

Mutex::Config& Mutex::Config::SetSharedMem()
{
    mShare = PTHREAD_PROCESS_SHARED;
    mRobust = PTHREAD_MUTEX_ROBUST;
    return *this;
}

Mutex::Config& Mutex::Config::SetSharedMemDestroy()
{
    mShare = PTHREAD_PROCESS_SHARED;
    mRobust = PTHREAD_MUTEX_ROBUST;
    mSharedMemDestroy = true;
    return *this;
}

Mutex::Config& Mutex::Config::SetRobust()
{
    mRobust = PTHREAD_MUTEX_ROBUST;
    return *this;
}

Mutex::Config& Mutex::Config::SetPrioInherit()
{
    mProto = PTHREAD_PRIO_INHERIT;
    return *this;
}

Mutex::Config& Mutex::Config::SetErrorCheck()
{
    mType = PTHREAD_MUTEX_ERRORCHECK;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::Init(const Mutex::Config& cfg)
{
    pthread_mutexattr_t pAttr;
    int rc = pthread_mutexattr_init(&pAttr);
    if (rc != 0)
        ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutexattr_init");

    /* Immediate exec - Need to destroy pthread attr object if failure occurs */
    const char* errorFunc = [this, &lrc = rc, &lpAttr = pAttr, &lcfg = cfg]() -> const char*
    {
        lrc = pthread_mutexattr_settype(&lpAttr, static_cast<int>(lcfg.mType));
        if (lrc != 0)
            return "pthread_mutexattr_settype";

        lrc = pthread_mutexattr_setpshared(&lpAttr, static_cast<int>(lcfg.mShare));
        if (lrc != 0)
            return "pthread_mutexattr_setpshared";

        lrc = pthread_mutexattr_setrobust(&lpAttr, static_cast<int>(lcfg.mRobust));
        if (lrc != 0)
            return "pthread_mutexattr_setrobust";

        lrc = pthread_mutexattr_setprotocol(&lpAttr, static_cast<int>(lcfg.mProto));
        if (lrc != 0)
            return "pthread_mutexattr_setprotocol";

        if (lcfg.mProto == PTHREAD_PRIO_PROTECT)
        {
            lrc = pthread_mutexattr_setprioceiling(&lpAttr, lcfg.mPrioCeiling);
            if (lrc != 0)
                return "pthread_mutexattr_setprioceiling";
        }

        lrc = pthread_mutex_init(mMutex, &lpAttr);
        if (lrc != 0)
            return "pthread_mutex_init";

        return nullptr;
    }();
    // Cleanup attribute
    pthread_mutexattr_destroy(&pAttr);

    if (rc != 0)
    {
        ThrowSystemError(mLog, mName, __func__, rc, errorFunc);
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
        // Repair is the caller's responsibility before the next Unlock();
        //  we make it consistent internally so the mutex remains usable.
        int rcConsistent = pthread_mutex_consistent(mMutex);
        if (rcConsistent == 0)
        {
            mLog->Info("{}: Mutex consistency restored on '{}'", __func__, mName);
        }
        else
        {
            ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutex_consistent failure");
        }
    }
    else
    {
        ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutex_lock");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::UnlockFail(int rc)
{
    ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutex_unlock");
}
}
