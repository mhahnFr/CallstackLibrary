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

#include <stdlib.h>

/**
 * @brief Allocates an uninitialized callstack object.
 *
 * @return A newly allocated callstack object.
 */
static inline struct callstack * callstack_allocate(void) {
    return malloc(sizeof(struct callstack));
}

/**
 * @brief Initializes the given callstack object.
 *
 * @param self The callstack object.
 */
static inline void callstack_create(struct callstack * self) {
    self->backtraceSize     = 0;
    self->stringArraySize   = 0;
    self->stringArray       = NULL;
    self->translationStatus = NONE;
}

/**
 * @brief Initializes the given callstack object using the given backtrace.
 *
 * The given callstack object has to have enough memory to hold the given backtrace.
 *
 * @param self The callstack object.
 * @param trace The backtrace, an array of return addresses.
 * @param traceLength The length of the trace array.
 */
void callstack_createWithBacktrace(struct callstack * self,
                                   void * trace[], size_t traceLength);

/**
 * @brief Calculates the length of all the strings inside of the translated callstack.
 *
 * Returns 0 if the callstack is not translated.
 *
 * @return The total length of the string array inside the callstack.
 */
size_t callstack_getTotalStringLength(struct callstack * self);

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
