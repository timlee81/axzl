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

    /**
     * Constructor
     *
     * @param name Name of logging component
     */
    StdOutLog(const char* name)
    : Log(name)
    {
    }

    /** Destructor */
    virtual ~StdOutLog() = default;

    /**
     * Wait until all log messages are pushed out
     */
    virtual void Drain() override;

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