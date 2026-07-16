/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"

namespace Axzl
{

class StdOutLog : public Log
{
public:
    // Deleted
    StdOutLog& operator=(StdOutLog& assign) = delete;
    StdOutLog& operator=(StdOutLog&& move) = delete;
    StdOutLog(StdOutLog& copy) = delete;
    StdOutLog(StdOutLog&& move) = delete;

    StdOutLog(const char* name)
    : Log(name)
    {
    }

    virtual ~StdOutLog() = default;

private:
    /**
     * Push expanded entry to output
     *      Takes over data
     *
     * @param hdr Header
     * @param msg Expanded message
     */
    virtual void PushEntry(std::string&& hdr, std::string&& msg) override;
};
}