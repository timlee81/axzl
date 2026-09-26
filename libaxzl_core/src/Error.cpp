/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Error.hpp"

#include "Log.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <stdlib.h>
#include <string>

namespace Axzl
{
/**
 * Was the Unrecoverable Error Policy specified in environment?
 *
 * @return UnrecoverableErrorPolicy if specified, otherwise none
 */
std::optional<UnrecoverableErrorPolicy> ErrorEnvPolicy()
{
    std::optional<UnrecoverableErrorPolicy> rc { std::nullopt };

    // secure_getenv is better security vs old-school getenv
    static auto envPolicy { secure_getenv("AXZL_UNRECOVERABLE_ERROR_POLICY") };
    if (!envPolicy)
    {
        std::string policy { envPolicy };
        std::transform(policy.begin(), policy.end(), policy.begin(),
            [](unsigned char c)
            { return std::tolower(c); });

        // Default to ignore environment
        if (policy == "throw")
            rc = UnrecoverableErrorPolicy::ThrowException;
        else if (policy == "logandthrow")
            rc = UnrecoverableErrorPolicy::LogAndThrowException;
        else if (policy == "logandcallhandler")
            rc = UnrecoverableErrorPolicy::LogAndCallHandler;
        else if (policy == "logandabort")
            rc = UnrecoverableErrorPolicy::logAndAbort;
        // else - ignored
    }
    return rc;
}

/**
 * Get the Unrecoverable Error Policy from the build configuration
 *
 * @return UnrecoverableError Build configuration value
 */
UnrecoverableErrorPolicy ErrorConfiguredPolicy()
{
    static auto axzlErrorPolicy =
#if defined(AXZL_UNRECOVERABLE_ERROR_POLICY_THROW)
        UnrecoverableErrorPolicy::ThrowException;
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_THROW)
        UnrecoverableErrorPolicy::LogAndThrowException;
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_CALL_HANDLER)
        UnrecoverableErrorPolicy::logAndCallHandler;
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_ABORT)
        UnrecoverableErrorPolicy::Abort;
#else
        UnrecoverableErrorPolicy::ThrowException;
#endif

    return axzlErrorPolicy;
}

/**
 * Get the Unrecoverable Error Policy from either environment or build configuration
 *
 * @return UnrecoverableErrorPolicy from environment or build configuration
 */
UnrecoverableErrorPolicy ErrorPolicy()
{
    static auto axzlEnvPolicy = ErrorEnvPolicy();
    static auto axzlErrorPolicy = axzlEnvPolicy ? axzlEnvPolicy.value() : ErrorConfiguredPolicy();
    return axzlErrorPolicy;
}

// void Throw(Error& err)
void Throw(LogPtr& log, const std::exception& exc)
{
    log->Critical("Exception! {}", exc.what());
    // TODO NEED FLUSH HERE before throw!!

    // Based on policy
    switch (ErrorPolicy())
    {
    case UnrecoverableErrorPolicy::ThrowException:
    case UnrecoverableErrorPolicy::LogAndThrowException:
        throw(exc);
        break;

    case UnrecoverableErrorPolicy::LogAndCallHandler:
        if (NoThrowHandler)
            NoThrowHandler(exc);
        else
            abort();
        break;

    case UnrecoverableErrorPolicy::logAndAbort:
        abort();
        break;
    }

    // superfluous abort
    abort();
}

/*
void Throw(const std::exception& exc)
{
    //Throw((log, exc);
}
*/

}