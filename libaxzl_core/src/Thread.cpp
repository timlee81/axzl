/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Thread.hpp"

#include "Error.hpp"

namespace Axzl
{
#if 0
///////////////////////////////////////////////////////////////////////////////////////////////////
void Thread::Init()
{
    pthread_attr_t pAttr;
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

        lrc = pthread_mutex_init(mMutex, &lpAttr);
        if (lrc != 0)
            return "pthread_mutex_init";

        return nullptr;
    }();
    // Cleanup attribute
    pthread_attr_destroy(&pAttr);

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
#endif

}
