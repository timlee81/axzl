/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "StdOutLog.hpp"

#include <iostream>

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
void StdOutLog::PushEntry(std::string&& hdr, std::string&& msg)
{
    // Dump! ... just stdout now... enqueue eventually
    std::cout << hdr << "..." << msg << "\n";
}

}