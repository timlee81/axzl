/*
 * SPDX-FileCopyrightText: 2026 Tim Lee, Axzl Project
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#if __cplusplus >= 201703L
// ---- C++17 and later: use the standard type ----
#include <string_view>
#endif

namespace Axzl
{

#if __cplusplus >= 201703L
// ---- C++17 and later: use the standard type ----
using string_view = std::string_view;

#else

#error oh no

#endif

}