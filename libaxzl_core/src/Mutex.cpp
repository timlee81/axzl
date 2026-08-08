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
void Mutex::Init()
{
    pthread_mutexattr_t pAttr;
    int rc = pthread_mutexattr_init(&pAttr);
    if (rc != 0)
        ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutexattr_init");

    /* Immediate exec - Need to destroy pthread attr object if failure occurs */
    const char* errorFunc = [this, &lrc = rc, &lpAttr = pAttr]() -> const char*
    {
        lrc = pthread_mutexattr_settype(&lpAttr, static_cast<int>(mMutexAttrs.type));
        if (lrc != 0)
            return "pthread_mutexattr_settype";

        lrc = pthread_mutexattr_setpshared(&lpAttr, static_cast<int>(mMutexAttrs.shared));
        if (lrc != 0)
            return "pthread_mutexattr_setpshared";

        lrc = pthread_mutexattr_setrobust(&lpAttr, static_cast<int>(mMutexAttrs.robust));
        if (lrc != 0)
            return "pthread_mutexattr_setrobust";

        lrc = pthread_mutexattr_setprotocol(&lpAttr, static_cast<int>(mMutexAttrs.proto));
        if (lrc != 0)
            return "pthread_mutexattr_setprotocol";

        if (mMutexAttrs.proto == MutexProtocol::Protect)
        {
            lrc = pthread_mutexattr_setprioceiling(&lpAttr, mMutexAttrs.priorityCeiling);
            if (lrc != 0)
                return "pthread_mutexattr_setprioceiling";
        }

        lrc = pthread_mutex_init(&mMutex, &lpAttr);
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
        mRobust = (mMutexAttrs.robust == MutexRobust::Robust);

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
        int rcConsistent = pthread_mutex_consistent(&mMutex);
        if (rcConsistent == 0)
        {
            rc = pthread_mutex_lock(&mMutex);
            if (rc != 0)
                ThrowSystemError(mLog, mName, __func__, rc, "Revived after EOWNERDEAD, but failed to re-lock");
        }
        else
        {
            ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutex_consistent failure");
            /*
            if (mLog)
            {
                mLog->Error("{}: [{}] Robust mutex EOWNERDEAD attempt to revive failed with '{}'",
                    __func__, mName, strerror(rcConsistent));
            }
            else
            {
                ThrowSystemError(mLog, mName, __func__, rc, "pthread_mutex_consistent failure");
                // throw std::system_error(rc, std::system_category(), "pthread_mutex_consistent failure");
            }
            */
        }
        return;
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
