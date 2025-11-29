/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#ifndef __lcs_callstack_h
#define __lcs_callstack_h

#include <stdbool.h>

#include "callstack_defs.h"
#include "callstack_frame.h"
#include "callstack_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a callstack.
 */
struct callstack {
    /** The type (status) of the translation to be human-readable. */
    enum callstack_type translationStatus;

    /**
     * The amount of available frames.
     *
     * @since v1.1
     */
    size_t frameCount;
    /**
     * An array of callstack frames.
     *
     * @since v1.1
     */
    struct callstack_frame * frames;
    /** The size of the backtrace.                                 */
    size_t  backtraceSize;
    /** The backtrace.                                             */
    void *  backtrace[CALLSTACK_BACKTRACE_SIZE];
};

/**
 * @brief Creates a callstack of the calling function.
 *
 * The backtrace of the calling function is created.<br>
 * The returned structure is allocated and needs to be freed using the function
 * @code callstack_delete(struct callstack *)@endcode.<br>
 * Returns @c NULL if an error occurs.
 *
 * @return A newly allocated callstack object.
 */
struct callstack * callstack_new(void);

/**
 * @brief Creates a new callstack object, ignoring all frames after the given
 * address.
 *
 * The returned structure is allocated and needs to be freed using the function
 * @code callstack_delete(struct callstack *)@endcode.<br>
 * Returns @c NULL if an error occurs.
 *
 * @param address The stack address after which frames are ignored.
 * @return A newly allocated callstack object.
 */
struct callstack* callstack_newWithAddress(const void* address);

/**
 * @brief Constructs the given callstack object.
 *
 * Stores the backtrace of the calling function.<br>
 * The callstack object needs to be destructed using the function
 * @code callstack_destroy(struct callstack *)@endcode upon successful
 * construction and use.<br>
 * If an error occurs during the initialization of the given callstack object,
 * @c false is returned.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @return Whether the given callstack object was constructed successfully.
 */
bool callstack_emplace(struct callstack * self);

/**
 * @brief Constructs the given callstack object.
 *
 * Stores the backtrace of the calling function, ignoring all frames after the
 * given address.<br>
 * The callstack object needs to be destructed using the function
 * @code callstack_destroy(struct callstack *)@endcode upon successful
 * construction and use.
 *
 * @param self A pointer to the callstack object to be constructed.
 * @param address The stack address after which frames are ignored.
 * @return Whether the given callstack object was constructed successfully.
 */
bool callstack_emplaceWithAddress(struct callstack* self, const void* address);

/**
 * @brief Constructs the given callstack object.
 *
 * Copies the given callstack into the given object. If the trace is longer
 * than @c CALLSTACK_BACKTRACE_SIZE, only the first addresses are copied.<br>
 * The callstack object needs to be destructed using the function
 * @code callstack_destroy(struct callstack *)@endcode after use.<br>
 * If the given trace length is smaller than zero, @c false is returned and the
 * given callstack is not modified.
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
 * @param self A pointer to the callstack to be replaced.
 * @param other The callstack object to be copied.
 */
void callstack_copy(struct callstack * self, const struct callstack * other);

bool callstack_relativize(struct callstack* self, const char** binaryNames);

struct callstack_frame* callstack_translateRelative(struct callstack* self, const char** binaryNames);

/**
 * @brief Translates the given callstack and returns an array of the translated frames.
 *
 * Returns @c NULL if an error happens.<br>
 * Since version @c 1.1 a @c callstack_frame array is returned.
 *
 * @param self The callstack object.
 * @return An array of translated callstack frames.
 */
struct callstack_frame * callstack_toArray(struct callstack * self);

/**
 * @brief Translates the given callstack and returns an array of the translated frames.
 *
 * If the given callstack has not been translated before, only the binary file
 * information is deducted.
 * <br><br>
 * Returns @c NULL if an error happens.
 *
 * @param self The callstack object.
 * @return An array of translated callstack frames.
 * @since v1.1
 */
struct callstack_frame * callstack_getBinaries(struct callstack * self);

#ifdef LCS_USE_UNSAFE_OPTIMIZATION
/**
 * @brief Translates the given callstack and returns an array of the translated frames.
 *
 * If the given has not been translated before, only the binary file
 * information is deducted.
 *
 * The deducted binary information is located in the cache of the library and
 * becomes invalid after a call to @c callstack_clearCaches() or after the
 * callstack has been translated entirely if @c callstack_autoClearCaches is
 * @c true .
 * <br><br>
 * Returns @c NULL if an error happens.
 *
 * @param self the callstack object
 * @return an array of translated callstack frames
 * @since v2.0
 */
struct callstack_frame* callstack_getBinariesCached(struct callstack* self);
#endif

/**
 * @brief Returns the number of frames stored in the given callstack.
 *
 * @param self The callstack object.
 * @return The number of frames in the given callstack.
 */
static inline size_t callstack_getFrameCount(const struct callstack* self) {
    return self->backtraceSize;
}

/**
 * @brief Returns the type of the given callstack.
 *
 * @param self The callstack object.
 * @return The type of the callstack.
 */
static inline enum callstack_type callstack_getType(const struct callstack* self) {
    return self->translationStatus;
}

/**
 * @brief Returns whether the given callstack is already translated.
 *
 * @param self The callstack object.
 * @return Whether the callstack is already translated.
 */
static inline bool callstack_isTranslated(const struct callstack* self) {
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

#endif /* __cplusplus */

#endif /* __lcs_callstack_h */
