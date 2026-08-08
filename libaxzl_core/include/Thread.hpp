/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include "Log.hpp"
#include "StringView.hpp"

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace Axzl
{

/**
 * Thread
 *
 * Maintain std::thread semantics, but allow pthread attribute setting.
 */
class Thread
{

public:
    /**
     * Constructor
     *  Create and run a new thread
     */
    template <typename Func, typename... Args>
    explicit Thread(string_view name,
        LogPtr log,
        /* Thread Parameters */
        /* priority, RR|FIFO*/
        Func&& func, Args&&... args)
    : mName(name.empty() ? "NotSmartThread" : name)
    , mLog(log ? log : GetLog())
    {
        /*
         * Bundle the callable and arguments into a tuple. Decay ensures we copy values
         *  rather than storing dangling refs to variables on the caller's stack.
         * Heap because it must outlive this call; the new thread may start running
         *  well after the constructor returns.
         */
        using CallData = std::tuple<std::decay_t<Func>, std::decay_t<Args>...>;
        auto callData = std::make_unique<CallData>(
            CallData(std::forward<Func>(func), std::forward<Args>(args)...));

        // Call pthread init here
        // PthreadInit();

        // pthread_t thread_id;
        int rc = pthread_create(&mTid, nullptr, &Thread::ThreadEntry<CallData>, callData.get());
        if (rc != 0)
        {
            // Throw()
            throw std::runtime_error("pthread_create failed");
        }

        /* New thread created, release callData to thread */
        callData.release();
        mJoin = true;
        mLog->Trace("{}:{} thread ctor", __func__, mName);
    }

    /** Join thread - block waiting for join to complete */
    void Join()
    {
        if (mJoin)
        {
            // Do join
            mLog->Debug("{}:{} begin join...", __func__, mName);
            void* threadRet;
            int rc = pthread_join(mTid, &threadRet);
            if (rc < 0)
                mLog->Error("{}:{} join error '{}'", __func__, mName, strerror(rc));
            else
                mLog->Debug("{}:{} joined", __func__, mName);

            mJoin = false;
        }
    }

    /** Destructor */
    ~Thread()
    {
        Join();
        mLog->Trace("{}:{} thread dtor", __func__, mName);
    }

    /** Not copyable */
    Thread(Thread& copy) = delete;
    Thread& operator=(Thread& assign) = delete;

    /** Moving is supported, pthread_t can be copied but not compared without pthread_equal */
    Thread(Thread&& from)
    : mName(std::move(from.mName))
    , mTid(from.mTid)
    , mLog(from.mLog)
    , mJoin(from.mJoin)
    {
        /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
        from.mJoin = false;
        from.mLog = nullptr;
    }

    Thread& operator=(Thread&& from)
    {
        if (this != &from)
        {
            mName = std::move(from.mName);
            mTid = from.mTid;
            mLog = from.mLog;
            mJoin = from.mJoin;
            /* Mark mJoin as false and old Thread won't do anything on Join/dtor */
            from.mJoin = false;
            from.mLog = nullptr;
        }
        return *this;
    }

private:
    /**
     * Thread Entry point - to avoid type-erasure, use a (small) template
     */
    template <typename CallData>
    static void* ThreadEntry(void* arg)
    {
        std::unique_ptr<CallData> callData(static_cast<CallData*>(arg));
        /* apply calls the lambda with the tuple args unpacked */
        std::apply(
            [](auto&&... unpacked)
            {
                /* invoke will handle the difference between standard function pointers
                    and member function pointers, else could have made the lambda
                    be [](auto&& f, auto&&... unpacked) { f((unpacked...));} */
                std::invoke(std::forward<decltype(unpacked)>(unpacked)...);
            },
            std::move(*callData));
        return nullptr;
    }

    /** Thread name - set in OS to be viewed by 'ps' */
    std::string mName;

    /** Logger */
    LogPtr mLog;

    /** Is the thread joinable? */
    bool mJoin { false };

    /** Thread ID */
    pthread_t mTid;
};
}

#if 0



    // Not copyable -- a pthread_t identifies a single OS thread.
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    // Movable.
    Thread(Thread&& other) noexcept
        : thread_id_(other.thread_id_), joinable_(other.joinable_) {
        other.joinable_ = false;
    }

    Thread& operator=(Thread&& other) noexcept {
        if (this != &other) {
            if (joinable_) {
                pthread_join(thread_id_, nullptr);
            }
            thread_id_ = other.thread_id_;
            joinable_ = other.joinable_;
            other.joinable_ = false;
        }
        return *this;
    }

    ~Thread() {
        // Mirrors std::thread's "must be joined or detached" contract
        // by joining automatically rather than calling std::terminate.
        if (joinable_) {
            pthread_join(thread_id_, nullptr);
        }
    }

    void join() {
        if (!joinable_) {
            throw std::runtime_error("Thread is not joinable");
        }
        int rc = pthread_join(thread_id_, nullptr);
        if (rc != 0) {
            throw std::runtime_error("pthread_join failed with code " +
                                      std::to_string(rc));
        }
        joinable_ = false;
    }

    bool joinable() const noexcept { return joinable_; }

#endif