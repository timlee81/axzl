/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include <chrono>
#include <time.h>

namespace Axzl
{

/**
 * Lookup POSIX clock_t from c++ clock
 *
 * @param tp Time point of clock to return
 * @return clock_t corresponding to the argument
 */
template <typename Clock, typename Duration>
constexpr clockid_t ClockId(const std::chrono::time_point<Clock, Duration>&)
{
    if constexpr (std::is_same_v<Clock, std::chrono::steady_clock>)
    {
        return CLOCK_MONOTONIC;
    }
    else if constexpr (std::is_same_v<Clock, std::chrono::system_clock>
        || std::is_same_v<Clock, std::chrono::high_resolution_clock>)
    {
        return CLOCK_REALTIME;
    }
    else
    {
        static_assert(sizeof(Clock) == 0, "Unsupported clock type for ClockId");
        return CLOCK_MONOTONIC; // Unreachable, but required for constexpr
    }
}

/**
 * Convert c++ time point to posix timespec
 *
 * @param tp Time point of clock to convertto timespec
 * @return posix timespec converted from tp argument
 */
template <typename Clock, typename Duration>
timespec DurationToTimespec(const std::chrono::time_point<Clock, Duration>& tp)
{
    using namespace std::chrono;

    // Convert timeout to posix abs time
    auto toDurNs = duration_cast<nanoseconds>(tp.time_since_epoch());
    auto secs = duration_cast<seconds>(toDurNs);
    auto nsecs = toDurNs - secs;

    // Handle negative ns case (can't be negative)
    if (nsecs.count() < 0)
    {
        secs -= seconds(1);
        nsecs += seconds(1);
    }

    // Store in ts
    timespec ts;
    ts.tv_sec = static_cast<time_t>(secs.count());
    ts.tv_nsec = static_cast<long>(nsecs.count());
    return ts;
}
}
