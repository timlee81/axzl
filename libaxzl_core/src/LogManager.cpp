/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "LogManager.hpp"

namespace Axzl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
LogManager& LogManager::Instance()
{
    static LogManager sInst;
    return sInst;
}
}
