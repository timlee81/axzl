/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include <exception>
#include <fmt/format.h>
#include <string_view>
#include <system_error>

namespace Axzl
{

enum class UnrecoverableErrorPolicy
{
    ThrowException,
    LogException,
    IgnoreException,
    Terminate,
};

// Forward declare Log
class Log;

/**
 * Exception occurred
 *
 * @param exc Exception
 */
void Throw(const std::exception& exc);

/**
 * Exception Occurred
 *
 * @param log Log to write to
 * @param exc Exception
 */
void Throw(Log& log, const std::exception& exc);

/**
 * Exception Occurred
 *
 * @param log Log to write to
 * @param exc Exception
 */
inline void ThrowSystemError(Log& log, std::string_view name, int rv, std::string_view what)
{
    Throw(log, std::system_error(rv, std::system_category(), fmt::format("{}: {}", name, what)));
}

// __func__ version
inline void ThrowSystemError(Log& log, std::string_view name, std::string_view where, int rv, std::string_view what)
{
    // Throw(log, std::system_error(rv, st  d::system_category(), std::string { name } + "@" + where ": " + what));
    Throw(log, std::system_error(rv, std::system_category(), fmt::format("{}@{}: {}", name, where, what)));
}

}
