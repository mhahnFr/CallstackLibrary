/*
* Callstack Library - A library creating human readable call stacks.
*
* Copyright (C) 2022  mhahnFr
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

#ifndef callstackinternal_h
#define callstackinternal_h

#include "../include/callstack.h"

/**
 * @brief Allocates an unintialized callstack object.
 *
 * The backtrace of the caller of this function is stored.
 *
 * @return A newly allocated callstack object or NULL in case of error.
 */
struct callstack * callstack_new();

/**
 * @brief Initializes the given callstack struct.
 *
 * @param self The callstack object.
 */
void callstack_create(struct callstack * self);

/**
 * @brief Destroys the given callstack object.
 *
 * The contents of the given object are invalidated.
 *
 * @param self The callstack object.
 */
void callstack_destroy(struct callstack * self);

/**
 * @brief Translates the given callstack object into a human readable format.
 *
 * Returns the status of the translation, which is also set into the given callstack object.
 *
 * @param self The callstack object.
 * @return The status of the translation.
 */
enum callstack_type callstack_translate(struct callstack * self);

#endif /* callstackinternal_h */
