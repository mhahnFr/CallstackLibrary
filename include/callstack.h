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

#ifndef callstack_h
#define callstack_h

#ifdef __cplusplus
extern "C" {
#endif

#include "../code/defs.h"
#include "callstack_type.h"

/**
 * A structure representing a callstack.
 */
struct callstack {
    /** The type (status) of the translation to be human readable. */
    enum callstack_type translationStatus;
    /** The size of the string array.                              */
    size_t  stringArraySize;
    /** The NULL terminated callstack string array.                */
    char ** stringArray;
    /** The size of the backtrace.                                 */
    size_t  backtraceSize;
    /** The backtrace.                                             */
    void *  backtrace[CALLSTACK_BACKTRACE_SIZE];
};

/**
 * @brief Creates a callstack of the calling function.
 *
 * The backtrace of the calling function is created and it tries to translate it into a human
 * readable format.
 * The struct is allocated and needs to be freed using the function callstack_delete(struct callstack *).
 *
 * @return A newly allocated callstack object.
 */
struct callstack * callstack_generate(void);

/**
 * @brief Constructs the given callstack object.
 *
 * Stores the backtrace of the calling function.
 *
 * @param self A pointer to the callstack object to be constructed.
 */
void callstack_emplace(struct callstack * self);

/**
 * @brief Constructs the given callstack object.
 *
 * Copies the given callstack into the given object. If the trace is longer than
 * CALLSTACK_BACKTRACE_SIZE, only the first addresses are copied.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @param trace The backtrace to be copied.
 * @param traceLength The length of the given trace.
 */
void callstack_emplaceWithBacktrace(struct callstack * self,
                                    void * trace[], size_t traceLength);

/**
 * @brief Creates an array of strings out of the backtrace and returns it.
 *
 * The backtrace is only deleted if it has not already been created.
 * The returned string array must not be freed.
 *
 * Returns NULL if an error happens.
 *
 * @param self The callstack object.
 * @return A string array consisting of human readable strings.
 */
char ** callstack_toArray(struct callstack * self);

/**
 * @brief Creates a single string out of the backtrace and returns it.
 *
 * The given seperator character is used to seperate the lines. The string
 * is allocated and needs to be freed.
 *
 * Returns NULL if an error happens.
 *
 * @param self The callstack object.
 * @param separator The separator to be used.
 * @return An allocated string consisting of the callstack.
 */
const char *  callstack_toString(struct callstack * self, char separator);

/**
 * @brief Returns the type of the callstack.
 *
 * @param self The callstack object.
 * @return The type of the callstack.
 */
static inline enum callstack_type callstack_getType(struct callstack * self) {
    return self->translationStatus;
}

/**
 * @brief Returns whether the callstack is already translated.
 *
 * @param self The callstack object.
 * @return Whether the callstack is already translated.
 */
static inline bool callstack_isTranslated(struct callstack * self) {
    return self->translationStatus != NONE && self->translationStatus != FAILED;
}

/**
 * @brief Deletes the given callstack.
 *
 * Destroys and deallocates the given callstack object.
 *
 * @param self The callstack object.
 */
void callstack_delete(struct callstack * self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_h */
