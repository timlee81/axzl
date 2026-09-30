/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "ConditionVariable.hpp"

#include "Error.hpp"

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
ConditionVariable::Config& ConditionVariable::Config::SetSharedMem()
{
    mShare = PTHREAD_PROCESS_SHARED;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ConditionVariable::Config& ConditionVariable::Config::SetSharedMemDestroy()
{
    mShare = PTHREAD_PROCESS_SHARED;
    mSharedMemDestroy = true;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ConditionVariable::ScopedPosixAttr::ScopedPosixAttr(string_view name, LogPtr log)
{
    int rc = pthread_condattr_init(&attr);
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_condattr_init failure");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ConditionVariable::ScopedPosixAttr::~ScopedPosixAttr()
{
    (void)pthread_condattr_destroy(&attr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ConditionVariable::Attributes::Attributes(string_view name, LogPtr log, const Config& cfg)
: mAttr(name, log)
{
    int rc = pthread_condattr_setpshared(&mAttr.attr, static_cast<int>(cfg.mShare));
    if (rc != 0)
        ThrowSystemError(name, log, __func__, rc, "pthread_condattr_setpshared");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void ConditionVariable::Init(const ConditionVariable::Config& cfg)
{
    // Scoped attribute will automatically clean-up via ScopedAttr
    Attributes attr { mName, mLog, cfg };

    int rc = pthread_cond_init(mCv, &attr.mAttr.attr);
    if (rc != 0)
    {
        ThrowSystemError(mName, mLog, __func__, rc, "pthread_cond_init");
    }
    else
    {
        mValid = true;
        mShared = (cfg.mShare == PTHREAD_PROCESS_SHARED);
        mSharedCleanup = cfg.mSharedMemDestroy;

        mLog->Debug("{}: Condvar init succeeded on '{}'", __func__, mName);
    }
}

}
