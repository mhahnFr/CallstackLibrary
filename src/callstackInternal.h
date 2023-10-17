/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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
#include "../include/callstack_create.h"

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
 * @brief Creates a backtrace into the given buffer.
 *
 * All frames upon the given address are removed from the generated backtrace.
 *
 * @param buffer the buffer to store the frame addresses in
 * @param bufferSize the count of available elements in the given buffer
 * @param address the address upon which frames are removed from the backtrace
 * @return the count of frame addresses stored in the given buffer
 */
int callstack_backtrace(void * buffer[], int bufferSize, void * address);

/**
 * @brief Translates the given callstack object into a human readable format.
 *
 * Returns the status of the translation, which is also set into the given callstack object.
 *
 * @param self The callstack object.
 * @return The status of the translation.
 */
enum callstack_type callstack_translate(struct callstack * self);

/**
 * Translates the callstack frames of the given callstack to their corresponding binary files.
 *
 * @param self the callstack object
 * @return whether the binaries where translated
 */
enum callstack_type callstack_translateBinaries(struct callstack * self);

/**
 * Removes all translated callstack frames from the given callstack object.
 *
 * @param self the callstack object
 */
void callstack_reset(struct callstack * self);

#endif /* callstackinternal_h */
