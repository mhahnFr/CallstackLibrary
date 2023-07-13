/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the CallstackLibrary. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef callstack_internals_h
#define callstack_internals_h

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Indicates whether the caches should be cleared automatically. */
extern bool callstack_autoClearCaches;

/**
 * @brief Clears the caches of this library.
 *
 * Only needs to be called when `callstack_autoClearCaches` is `false`.
 */
void callstack_clearCaches(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_internals_h */
