/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#ifndef AXZL_LIBAXZL_CORE_INCLUDE_LIKELY_H
#define AXZL_LIBAXZL_CORE_INCLUDE_LIKELY_H

/**
 * Branch prediction hints: expands Likely and Unlikely for common compilers
 * Supports GCC, Clang and MSVC. Falls back to identity on unknown compilers.
 *
 * Usage:
 *      if (Likely(x)) { ... }
 *      if (Unlikely(x)) { ... }
 */

// Detect C++20 or newer
#if __cplusplus >= 202002L

// In C++20, we safely evaluate 'x' and tag the path block right after it
#define Likely(x) (x)
#define Unlikely(x) (x)

#else // C++11 / C++14 / C++17 fallback

// Prefer compiler builtins when available
#if defined(__has_builtin)
#if __has_builtin(__builtin_expect)
#define Likely(x) (__builtin_expect(!!(x), 1))
#define Unlikely(x) (__builtin_expect(!!(x), 0))
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
// GCC and Clang intrinsics for older standards
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
// Fallback for compilers like MSVC that lack direct C++11 branch weights
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#endif

#if !defined(Likely)
#if defined(__GNUC__) || defined(__clang__)
#define Likely(x) (__builtin_expect(!!(x), 1))
#define Unlikely(x) (__builtin_expect(!!(x), 0))
#elif defined(_MSC_VER)
// MSVC does not have __builtin_expect. Use plain expression evaluation.
// More advanced MSVC hints (like __assume) are statement-level and not
// suitable for expression macros in a portable header.
#define Likely(x) (x)
#define Unlikely(x) (x)
#else
#define Likely(x) (x)
#define Unlikely(x) (x)
#endif
#endif

#endif // AXZL_LIBAXZL_CORE_INCLUDE_LIKELY_H
