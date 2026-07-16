/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Likely.h"

#include <array>
#include <atomic>
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <utility>

namespace Axzl
{
class Log;
/**
 * Get default log
 */
std::shared_ptr<Log>& GetLog();

// ADD - immediate stdout log (no queue), queue stdout, queue systemd

// CHNAGE THESE to expected monadic uses!!!

/**
 * Pre-processor macros for logging
 *
 * These are primarily useful in performance critical code as normal
 * method calls will expand arguments, which may be runtime expensive.
 */
#define AxzlTrace(inst, ...)                                               \
    do                                                                     \
    {                                                                      \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Trace) [[unlikely]] \
            inst.Trace(__VA_ARGS__);                                       \
    } while (0)

#define AxzlDebug(inst, ...)                                               \
    do                                                                     \
    {                                                                      \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Debug) [[unlikely]] \
            inst.Debug(__VA_ARGS__);                                       \
    } while (0)

#define AxzlInfo(inst, ...)                                               \
    do                                                                    \
    {                                                                     \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Info) [[unlikely]] \
            inst.Info(__VA_ARGS__);                                       \
    } while (0)

#define AxzlWarn(inst, ...)                                               \
    do                                                                    \
    {                                                                     \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Warn) [[unlikely]] \
            inst.Warn(__VA_ARGS__);                                       \
    } while (0)

#define AxzlError(inst, ...)                                               \
    do                                                                     \
    {                                                                      \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Error) [[unlikely]] \
            inst.Error(__VA_ARGS__);                                       \
    } while (0)

#define AxzlCritical(inst, ...)                                               \
    do                                                                        \
    {                                                                         \
        if (Unlikely(inst.Level() >= Axzl::Log::Level::Critical) [[unlikely]] \
            inst.Critical(__VA_ARGS__);                                       \
    } while (0)

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

    virtual ~Log() = default;

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
    // Not sure these are really necessary... compiler will generate 0-arg versions
    /*
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
*/
protected:
    /**
     * Make Header from expanded flags
     *
     * @return Header
     */
    std::string MakeHeader(Level level);

    /**
     * Push expanded entry to output
     *      Takes over data
     *
     * @param hdr Header
     * @param msg Expanded message
     */
    virtual void PushEntry(std::string&& hdr, std::string&& msg) = 0;

    /**
     * Expand Log - templated
     * Deep copy for thread bounce
     *
     * @param level Level
     * @param logFmt Fmt string
     * @param args Fmt string arguments
     */
    template <typename... Args>
    void ExpandLogMsg(Level level, fmt::format_string<Args...>& logFmt, Args&&... args)
    {
        if (mSkipAllLogging)
            return;

        // Expand header
        std::string hdr { MakeHeader(level) };
        std::string msg { fmt::format(logFmt, args...) };
        PushEntry(std::move(hdr), std::move(msg));
    }

    /**
     * Expand Log - non-templated
     * Deep copy for thread bounce
     *
     * @param level Level
     * @param logMsg Msg to expand
     */
    void ExpandLogMsg(Level level, std::string_view logMsg)
    {
        if (mSkipAllLogging)
            return;

        std::string hdr { MakeHeader(level) };
        std::string msg { logMsg };
        PushEntry(std::move(hdr), std::move(msg));
    }

protected:
    /** Skip all logging if target is empty */
    bool mSkipAllLogging { false };

private:
    /**
     * Print Level
     *
     * @return Printed level
     */
    static constexpr std::string_view LevelToString(Level level) noexcept
    {
        return LEVEL_STRINGS[static_cast<size_t>(level)];
    }

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

    /** Level strings */
    static constexpr std::array<const char*, 7> LEVEL_STRINGS {
        "Trace", // LOG_DEBUG
        "Debug", // LOG_DEBUG
        "Info", // LOG_INFO
        "Warn", // LOG_WARNING
        "Error", // LOG_ERR
        "Critical", // LOG_CRIT
        "Off", // LOG_INFO
    };
};
}