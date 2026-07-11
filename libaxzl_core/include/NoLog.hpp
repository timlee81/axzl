/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"

namespace Axzl
{

class NoLog : public Log
{
public:
    // Deleted
    NoLog& operator=(NoLog& assign) = delete;
    NoLog& operator=(NoLog&& move) = delete;
    NoLog(NoLog& copy) = delete;
    NoLog(NoLog&& move) = delete;

    NoLog(const char* name = nullptr)
    : Log(name)
    {
        // Disable all logging in Log
        mSkipAllLogging = true;
    }

private:
    /**
     * Push expanded entry to output
     *      Takes over data
     *
     * @param hdr Header
     * @param msg Expanded message
     */
    virtual void PushEntry(std::string&& hdr, std::string&& msg) override
    {
        // Won't even get called due to mSkipAllLogging in Log
    }
};

}