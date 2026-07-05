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
    int rv = pthread_mutexattr_init(&pAttr);
    if (rv != 0)
        ThrowSystemError(*mLog, mName, __func__, rv, "pthread_mutexattr_init");

    /* Immediate exec - Need to destroy pthread attr object if failure occurs */
    const char* errorFunc = [this, &lrv = rv, &lpAttr = pAttr]() -> const char*
    {
        lrv = pthread_mutexattr_settype(&lpAttr, static_cast<int>(mMutexAttrs.type));
        if (lrv != 0)
            return "pthread_mutexattr_settype";

        lrv = pthread_mutexattr_setpshared(&lpAttr, static_cast<int>(mMutexAttrs.shared));
        if (lrv != 0)
            return "pthread_mutexattr_setpshared";

        lrv = pthread_mutexattr_setrobust(&lpAttr, static_cast<int>(mMutexAttrs.robust));
        if (lrv != 0)
            return "pthread_mutexattr_setrobust";

        lrv = pthread_mutexattr_setprotocol(&lpAttr, static_cast<int>(mMutexAttrs.proto));
        if (lrv != 0)
            return "pthread_mutexattr_setprotocol";

        if (mMutexAttrs.proto == MutexProtocol::Protect)
        {
            lrv = pthread_mutexattr_setprioceiling(&lpAttr, mMutexAttrs.priorityCeiling);
            if (lrv != 0)
                return "pthread_mutexattr_setprioceiling";
        }

        lrv = pthread_mutex_init(&mMutex, &lpAttr);
        if (lrv != 0)
            return "pthread_mutex_init";

        return nullptr;
    }();
    // Cleanup attribute
    pthread_mutexattr_destroy(&pAttr);

    if (rv != 0)
    {
        ThrowSystemError(*mLog, mName, __func__, rv, errorFunc);
    }
    else
    {
        mValid = true;
        mRobust = (mMutexAttrs.robust == MutexRobust::Robust);

        mLog->Debug("{}: Mutex init succeeded on '{}'", __func__, mName);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::LockFail(int rv)
{
    if (rv == EOWNERDEAD && mRobust)
    {
        // Previous owner died (EOWNERDEAD) — mutex is locked but marked inconsistent.
        // Repair is the caller's responsibility before the next Unlock();
        //  we make it consistent internally so the mutex remains usable.
        int rvConsistent = pthread_mutex_consistent(&mMutex);
        if (rvConsistent == 0)
        {
            rv = pthread_mutex_lock(&mMutex);
            if (rv != 0)
                ThrowSystemError(*mLog, mName, __func__, rv, "Revived after EOWNERDEAD, but failed to re-lock");
        }
        else
        {
            ThrowSystemError(*mLog, mName, __func__, rv, "pthread_mutex_consistent failure");
            /*
            if (mLog)
            {
                mLog->Error("{}: [{}] Robust mutex EOWNERDEAD attempt to revive failed with '{}'",
                    __func__, mName, strerror(rvConsistent));
            }
            else
            {
                ThrowSystemError(*mLog, mName, __func__, rv, "pthread_mutex_consistent failure");
                // throw std::system_error(rv, std::system_category(), "pthread_mutex_consistent failure");
            }
            */
        }
        return;
    }
    else
    {
        ThrowSystemError(*mLog, mName, __func__, rv, "pthread_mutex_lock");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::UnlockFail(int rv)
{
    ThrowSystemError(*mLog, mName, __func__, rv, "pthread_mutex_unlock");
}
}
