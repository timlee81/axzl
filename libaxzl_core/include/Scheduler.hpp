/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include <limits>
#include <pthread.h>

namespace Axzl
{

// template <typename SchedPolicy, int SchedPrio>
struct SchedulerParams
{
    enum class Policy
    {
        RealtimeFifo = SCHED_FIFO,
        RealtimeRoundRobin = SCHED_RR,
        RealtimeDeadline = SCHED_DEADLINE,
        Fair = SCHED_OTHER,
        Idle = SCHED_IDLE,
        Batch = SCHED_BATCH,
    };

    /** Policy */
    Policy policy;

    /** Either RT prio or Nice value */
    int prio;

    /**
     * Constructor
     * Must define both parameters
     */
    constexpr SchedulerParams(Policy pol, int pr)
    : policy { pol }
    , prio { pr }
    {
    }

    /** Is this a real-time policy */
    constexpr bool IsRt() const
    {
        switch (policy)
        {
        case Policy::RealtimeFifo:
        case Policy::RealtimeRoundRobin:
        case Policy::RealtimeDeadline:
            return true;

        default:
            return false;
        }
    }
};

constexpr SchedulerParams SchedFair { SchedulerParams::Policy::Fair, 0 };
constexpr SchedulerParams SchedInterrupt { SchedulerParams::Policy::RealtimeRoundRobin, 60 };

}
