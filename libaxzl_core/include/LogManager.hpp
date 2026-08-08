/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"
#include "StringView.hpp"

#include <memory>

namespace Axzl
{
class Log;

// template <typename OutType>
class LogManager
{

public:
    /** Get Instance */
    static LogManager& Instance();

    /** No copying/moving */
    LogManager(LogManager& copy) = delete;
    LogManager(LogManager&& move) = delete;
    LogManager& operator=(LogManager& assign) = delete;
    LogManager& operator=(LogManager&& assign) = delete;

    void AddLog(Log& log);
    LogPtr GetLog(string_view sv);

private:
    /** Constructor */
    LogManager();

    // OutType& GetOutType();
    //    LogPtr GetLog(string_view sv, OutType& out);
};

// using SysdLogManager = LogManager<std::SysdLog>;

}