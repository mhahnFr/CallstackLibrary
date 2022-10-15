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

#include "callstack_type.h"
#include "callstack_defs.h"

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
 * The backtrace of the calling function is created.
 * The struct is allocated and needs to be freed using the function callstack_delete(struct callstack *).
 * Returns NULL if an error occurs.
 *
 * @return A newly allocated callstack object.
 */
struct callstack * callstack_new(void);

/**
 * @brief Creates a new callstack object, ignoring all frames after the given address.
 *
 * The struct is allocated and needs to be freed using the function callstack_delete(struct callstack *).
 * Returns NULL if an error occurs.
 *
 * @param address The stack address after which frames are ignored.
 * @return A newly allocated callstack object.
 */
struct callstack * callstack_newWithAddress(void * address);

/**
 * @brief Constructs the given callstack object.
 *
 * Stores the backtrace of the calling function.
 * The callstack object needs to be destructed using the function callstack_destroy(struct callstack *)
 * upon successfull construction and use.
 * If an error occurs during the initialization of the given callstack object, false is returned.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @return Whether the given callstack object was constructed successfully.
 */
bool callstack_emplace(struct callstack * self);

/**
 * @brief Constructs the given callstack object.
 *
 * Stores the backtrace of the calling function, ignoring all frames after the given address.
 * The callstack object needs to be destructed using the function callstack_destroy(struct callstack *)
 * upon successfull construction and use.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @param address The stack address after which frames are ignored.
 * @return Whether the given callstack object was constructed successfully.
 */
bool callstack_emplaceWithAddress(struct callstack * self, void * address);

/**
 * @brief Constructs the given callstack object.
 *
 * Copies the given callstack into the given object. If the trace is longer than
 * CALLSTACK_BACKTRACE_SIZE, only the first addresses are copied.
 * The callstack object needs to be destructed using the function callstack_destroy(struct callstack *)
 * after use.
 * If the fiven trace length is smaller than zero, false is returned and the given callstack
 * is not modified.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @param trace The backtrace to be copied.
 * @param traceLength The length of the given trace.
 * @return Whether the given callstack object was constructed successfully.
 */
bool callstack_emplaceWithBacktrace(struct callstack * self,
                                    void * trace[], int traceLength);

/**
 * @brief Copies the given callstack.
 *
 * The given callstack is destroyed before the contents of the other one are copied.
 *
 * @param self A pointer to the the callstack to be replaced.
 * @param other The callstack object to be copied.
 */
void callstack_copy(struct callstack * self, const struct callstack * other);

/**
 * @brief Creates an array of strings out of the backtrace and returns it.
 *
 * The backtrace is only constructed if it has not already been created.
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
 * @brief Returns the number of frames stored in the given callstack.
 *
 * @param self The callstack object.
 * @return The number of frames in the given callstack.
 */
static inline size_t callstack_getFrameCount(struct callstack * self) {
    return self->backtraceSize;
}

/**
 * @brief Returns the type of the given callstack.
 *
 * @param self The callstack object.
 * @return The type of the callstack.
 */
static inline enum callstack_type callstack_getType(struct callstack * self) {
    return self->translationStatus;
}

/**
 * @brief Returns whether the given callstack is already translated.
 *
 * @param self The callstack object.
 * @return Whether the callstack is already translated.
 */
static inline bool callstack_isTranslated(struct callstack * self) {
    return self->translationStatus != NONE && self->translationStatus != FAILED;
}

/**
 * @brief Destroys the given callstack object.
 *
 * The contents of the given object are invalidated.
 *
 * @param self The callstack object.
 */
void callstack_destroy(struct callstack * self);

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

#include "callstack.hpp"

#endif

#endif /* callstack_h */
