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
    LogAndThrowException,
    LogAndCallHandler,
    logAndAbort,
};

// Forward declare Log
class Log;

/**
 * Exception occurred
 *
 * @param exc Exception
 */
[[noreturn]] void Throw(const std::exception& exc);

/**
 * Exception Occurred
 *
 * @param log Log to write to
 * @param exc Exception
 */
[[noreturn]] void Throw(LogPtr& log, const std::exception& exc);

/**
 * Exception Occurred
 *
 * @param log Log to write to
 * @param exc Exception
 */
[[noreturn]] inline void ThrowSystemError(string_view name, LogPtr log, int rc, string_view what)
{
    Throw(log, std::system_error(rc, std::system_category(), fmt::format("{}: {}", name, what)));
}

// __func__ version
[[noreturn]] inline void ThrowSystemError(string_view name, LogPtr log, string_view where, int rc, string_view what)
{
    Throw(log, std::system_error(rc, std::system_category(), fmt::format("{}@{}: {}", name, where, what)));
}
#define AXZL_THROW_SYS_ERROR(name, log, rc, what) \
    ThrowSystemError(name, log, __func__ __LINE__, rc, what)

/**
 * Override this if not throwing exceptions
 */
[[noreturn]] void __attribute__((weak)) NoThrowHandler(const std::exception& exc);

}
