/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Log.hpp"

#include <iostream>

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string Log::MakeHeader(Level level)
{
    auto hdr = fmt::memory_buffer();
    // Clocks go first...
    // Clock
    // Optional Clock (monotonic)
    fmt::format_to(std::back_inserter(hdr), "[{}]-", LevelToString(level));
    // Supplemental flags (pid, tid)

    return fmt::to_string(hdr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/*
void Log::PushEntry(std::string&& hdr, std::string&& msg)
{
    // Dump! ... just stdout now... enqueue eventually
    std::cout << hdr << "..." << msg << "\n";
}
*/

// Must copy to be handled on other thread!

// Expand

// vdso??? don't snap clock if not!?

//}

// stdout
// stdout
// systemd

// STOP repeated messages and token-bucket print them
}