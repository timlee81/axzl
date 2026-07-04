/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include <exception>

namespace Axzl
{

enum class UnrecoverableErrorPolicy
{
    ThrowException,
    LogException,
    IgnoreException,
    Terminate,
};

/**
 * Exception Occurred
 *
 * @param exc Exception
 */
void Throw(std::exception exc);

}
