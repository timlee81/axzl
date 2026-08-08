/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"
#include "StringView.hpp"

#include <exception>
#include <fmt/format.h>
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
void Throw(LogPtr& log, const std::exception& exc);

/**
 * Exception Occurred
 *
 * @param log Log to write to
 * @param exc Exception
 */
inline void ThrowSystemError(LogPtr& log, string_view name, int rv, string_view what)
{
    Throw(log, std::system_error(rv, std::system_category(), fmt::format("{}: {}", name, what)));
}

// __func__ version
inline void ThrowSystemError(LogPtr& log, string_view name, string_view where, int rv, string_view what)
{
    Throw(log, std::system_error(rv, std::system_category(), fmt::format("{}@{}: {}", name, where, what)));
}

}
