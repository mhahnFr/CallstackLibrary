/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2025 - 2026  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _lcs_symbols_symbolInfo_h
#define _lcs_symbols_symbolInfo_h

#include "../callstack_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

struct callstack_frame symbols_getInfo(const void* address, bool* success);

#ifdef LCS_USE_UNSAFE_OPTIMIZATION
struct callstack_frame symbols_getInfoCached(const void* address, bool* success);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _lcs_symbols_symbolInfo_h