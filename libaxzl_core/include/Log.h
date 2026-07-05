/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Likely.h"

#include <atomic>
#include <fmt/format.h>
#include <string>
#include <utility>

namespace Axzl
{
class Log;
/**
 * Get default log
 */
std::shared_ptr<Log>& GetLog();

/**
 * Pre-processor macros for logging
 *
 * These are primarily useful in performance critical code as normal
 * method calls will expand arguments, which may be runtime expensive.
 */
#define AxzlTrace(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Trace) [[unlikely]] \
        inst.Trace(...)

#define AxzlDebug(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Debug) [[unlikely]] \
        inst.Debug(...)

#define AxzlInfo(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Info) [[unlikely]] \
        inst.Info(...)

#define AxzlWarn(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Warn) [[unlikely]] \
        inst.Warn(...)

#define AxzlError(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Error) [[unlikely]] \
        inst.Error(...)

#define AxzlCritical(inst, ...)                                           \
    if (Unlikely(inst.Level() >= Axzl::Log::Level::Critical) [[unlikely]] \
        inst.Critical(...)

/**
 * Axzl Logging API
 *
 * Inherently multi-threaded
 */
class Log
{
public:
    /** Log levels (inspired by spdlog) */
    enum class Level
    {
        // clang-format off
        // spdlog-inspired level    syslog/systemd level
        Trace,                      // LOG_DEBUG
        Debug,                      // LOG_DEBUG
        Info,                       // LOG_INFO
        Warn,                       // LOG_WARNING
        Error,                      // LOG_ERR
        Critical,                   // LOG_CRIT
        Off,                        // LOG_INFO
        Max,
        // clang-format on
    };

    // Deleted
    Log& operator=(Log& assign) = delete;
    Log& operator=(Log&& move) = delete;
    Log(Log& copy) = delete;
    Log(Log&& move) = delete;

    Log(const char* name)
    : mName(name)
    {
    }

    void SetLevel(Level level)
    {
        mLevel.store(level, std::memory_order::memory_order_relaxed);
    }

    Level GetLevel()
    {
        return mLevel.load(std::memory_order::memory_order_relaxed);
    }

    template <typename... Args>
    void Critical(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Critical)) [[unlikely]]
            ExpandLogMsg(Level::Critical, logFmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Error(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Error)) [[unlikely]]
            ExpandLogMsg(Level::Error, logFmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Warn(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Warn)) [[unlikely]]
            ExpandLogMsg(Level::Warn, logFmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Info(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Info)) [[unlikely]]
            ExpandLogMsg(Level::Info, logFmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Debug(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Debug)) [[unlikely]]
            ExpandLogMsg(Level::Debug, logFmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void Trace(fmt::format_string<Args...> logFmt, Args&&... args)
    {
        if (Unlikely(GetLevel() >= Level::Trace)) [[unlikely]]
            ExpandLogMsg(Level::Trace, logFmt, std::forward<Args>(args)...);
    }

    /* No argument versions */

    void Critical(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Critical)) [[unlikely]]
            ExpandLogMsg(Level::Critical, logMsg);
    }

    void Error(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Error)) [[unlikely]]
            ExpandLogMsg(Level::Error, logMsg);
    }

    void Warn(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Warn)) [[unlikely]]
            ExpandLogMsg(Level::Warn, logMsg);
    }

    void Info(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Info)) [[unlikely]]
            ExpandLogMsg(Level::Info, logMsg);
    }

    void Debug(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Debug)) [[unlikely]]
            ExpandLogMsg(Level::Debug, logMsg);
    }

    void Trace(const char* logMsg)
    {
        if (Unlikely(GetLevel() >= Level::Trace)) [[unlikely]]
            ExpandLogMsg(Level::Trace, logMsg);
    }

protected:
    template <typename... Args>
    void ExpandLogMsg(Level level, fmt::format_string<Args...>& logFmt, Args&&... args)
    {
        auto msg = fmt::memory_buffer();
        // fmt::format_to(std::back_inserter(msg), )
        // Add '[level]' to begin
        fmt::format_to(std::back_inserter(msg), logFmt, args...);
    }

    void ExpandLogMsg(Level level, const char* logMsg)
    {
        auto msg = fmt::memory_buffer();
        // fmt::format_to(std::back_inserter(msg), )
        // Add '[level]' to begin
        fmt::format_to(std::back_inserter(msg), logMsg);
    }

    void ExpandLogMsg(Level, fmt::memory_buffer& msg);

    //--
    // Print ["Level"]
    // Add Pid, Tid options
    // Add Clock source option

private:
    /** Log name */
    std::string mName;

    /** Current log level */
    std::atomic<Level> mLevel = Level::Off;

    /** Print PID as part of log entry */
    bool mPid { false };

    /** Print TID as part of log entry */
    bool mTid { false };

    /** Print supplemental clock time as part of log entry */
    bool mExtraClock { false };
};
}