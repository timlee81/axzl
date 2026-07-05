/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Error.hpp"

#include "Log.h"

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
    std::optional<UnrecoverableErrorPolicy> rv { std::nullopt };

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
            rv = UnrecoverableErrorPolicy::ThrowException;
        else if (policy == "log")
            rv = UnrecoverableErrorPolicy::LogException;
        else if (policy == "ignore")
            rv = UnrecoverableErrorPolicy::IgnoreException;
        else if (policy == "terminate")
            rv = UnrecoverableErrorPolicy::Terminate;
        // else - ignored
    }
    return rv;
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
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG)
        UnrecoverableErrorPolicy::LogException;
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_IGNORE)
        UnrecoverableErrorPolicy::IgnoreException;
#elif defined(AXZL_UNRECOVERABLE_ERROR_POLICY_TERMINATE)
        UnrecoverableErrorPolicy::Terminate;
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
void Throw(Log& log, const std::exception& exc)
{
    // Based on policy
    switch (ErrorPolicy())
    {
    case UnrecoverableErrorPolicy::ThrowException:
        throw(exc);
        break;
    case UnrecoverableErrorPolicy::LogException:
        // Log
        // TODO add logging feature
        break;
    case UnrecoverableErrorPolicy::IgnoreException:
        break;
    case UnrecoverableErrorPolicy::Terminate:
        std::terminate();
        break;
    }
}

/*
void Throw(const std::exception& exc)
{
    //Throw((log, exc);
}
*/

}