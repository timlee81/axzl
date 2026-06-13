/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Mutex.hpp"

#include <pthread.h>

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::Init(const MutexAttributes& attrs)
{
    pthread_mutexattr_t pattr;
    int rv;
    const char* errorFunc = nullptr;

    errorFunc = [this, &rv, &pattr, &attrs]()
    {
        rv = pthread_mutexattr_init(&pattr);
        if (rv != 0)
            return "pthread_mutexattr_init";

        rv = pthread_mutexattr_settype(&pattr, static_cast<int>(attrs.type));
        if (rv != 0)
            return "pthread_mutexattr_settype";

        rv = pthread_mutexattr_setpshared(&pattr, static_cast<int>(attrs.shared));
        if (rv != 0)
            return "pthread_mutexattr_setpshared";

        rv = pthread_mutexattr_setrobust(&pattr, static_cast<int>(attrs.robust));
        if (rv != 0)
            return "pthread_mutexattr_setrobust";

        rv = pthread_mutexattr_setprotocol(&pattr, static_cast<int>(attrs.proto));
        if (rv != 0)
            return "pthread_mutexattr_setprotocol";

        if (attrs.proto == MutexProtocol::Protect)
        {
            rv = pthread_mutexattr_setprioceiling(&pattr, attrs.priorityCeiling);
            if (rv != 0)
                return "pthread_mutexattr_setprioceiling";
        }

        rv = pthread_mutex_init(&mMutex, &pattr);
        if (rv != 0)
            return "pthread_mutex_init";

        mValid = true;
        pthread_mutexattr_destroy(&pattr);
        return "";
    }(); // Execute

    if (rv != 0)
        pthread_mutexattr_destroy(&pattr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::LockFail(int rv)
{
    if (rv == EOWNERDEAD && mRobust)
    {
        // Previous owner died — mutex is locked but marked inconsistent.
        // Repair is the caller's responsibility before the next Unlock();
        // we make it consistent internally so the mutex remains usable.
        (void)Consistent();

        // TRY LOCK AGAIN
        return;
    }

    if (rv == EOWNERDEAD)
        (void)Consistent();
    //            if (mLog)
    // throw std::system_error(rv, std::system_category(), "pthread_mutex_lock");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void Mutex::UnlockFail(int rv)
{
    {
        //            if (mLog)
        // throw std::system_error(rv, std::system_category(), "pthread_mutex_lock");
    }
}

}
