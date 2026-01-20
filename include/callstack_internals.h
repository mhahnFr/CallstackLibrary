/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#ifndef _lcs_callstack_internals_h
#define _lcs_callstack_internals_h

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Indicates to the symbolizer whether to make function names human-readable.
 *
 * @since v2.1
 */
extern bool callstack_rawNames;

/**
 * Indicates whether the caches should be cleared automatically.
 *
 * @since v1.1
 */
extern bool callstack_autoClearCaches;

/**
 * @brief Clears the caches of this library.
 *
 * Only needs to be called when @c callstack_autoClearCaches is @c false .
 *
 * @since v1.1
 */
void callstack_clearCaches(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _lcs_callstack_internals_h */
