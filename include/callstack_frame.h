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

#ifndef __lcs_callstack_frame_h
#define __lcs_callstack_frame_h

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents a translated callstack frame.
 *
 * @since v1.1
 */
struct callstack_frame {
    /**
     * Reserved value.
     *
     * @since v2.0
     */
    void* reserved;
    /**
     * Reserved value.
     *
     * @since v2.0
     */
    bool  reserved1;
    /**
     * Reserved value.
     *
     * @since v2.2
     */
    bool  reserved2;

    /**
     * Indicates whether the source file was detected to be changed after it has
     * been used as source file for this callstack frame.
     *
     * @since v1.2
     */
    bool sourceFileOutdated;
    /**
     * Indicates whether the binary file is the CallstackLibrary itself.
     *
     * @since v2.0
     */
    bool binaryFileIsSelf;
    /** The name of the binary file this frame is in.        */
    char * binaryFile;
    /** The relative path of the name of the binary file.    */
    char * binaryFileRelative;
    /** The name of the function this frame is in.           */
    char * function;
    /** The name of the source file this frame is in.        */
    char * sourceFile;
    /** The relative path of the name of the source file.    */
    char * sourceFileRelative;
    /** The line number in the source file this frame is on. */
    unsigned long sourceLine;
    /** The line column number in the source file.           */
    unsigned long sourceLineColumn;
};

#ifdef __cplusplus
# define callstack_frame_initializer_prefix
#else
# define callstack_frame_initializer_prefix (struct callstack_frame)
#endif

#define callstack_frame_initializer \
    callstack_frame_initializer_prefix { NULL, false, false, false, false, NULL, NULL, NULL, NULL, NULL, 0, 0 }

/**
 * @brief Constructs the given callstack frame.
 *
 * @details Since version 2.X this function has been replaced by a macro.
 *
 * @param self the callstack frame to be initialized
 * @since v1.1
 * @deprecated Prefer using the macro @c callstack_frame_initializer .
 */
#define callstack_frame_create(self) (*(struct callstack_frame*) (self) = callstack_frame_initializer)

/**
 * Allocates a new and initialized callstack frame.
 *
 * @return the allocated callstack frame or @c NULL if unable to allocate
 * @since v1.1
 */
struct callstack_frame* callstack_frame_new(void);

/**
 * Allocates a new callstack frame and deeply copies the given callstack frame.
 *
 * @param self the callstack frame to be copied
 * @return a copy of the given callstack frame or @c NULL if unable to allocate
 * @since v1.1
 */
struct callstack_frame * callstack_frame_copy(const struct callstack_frame * self);

/**
 * Copies the given callstack frame into the given destination.
 *
 * @param destination the callstack frame filled with the copy
 * @param source the callstack frame to be copied
 * @since v1.1
 */
void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source);

/**
 * Returns the shortest binary file name of the given callstack frame.
 *
 * @param self the callstack frame
 * @return the shortest name
 * @since v1.1
 */
char * callstack_frame_getShortestName(const struct callstack_frame * self);

/**
 * @brief Returns the shortest binary file name of the given callstack frame.
 *
 * If the given callstack frame does not have a binary file name the given
 * fallback is returned.
 *
 * @param self the callstack frame
 * @param fallback the fallback string to be returned
 * @return the shortest binary file name or the given fallback
 * @since v1.2
 */
static inline const char* callstack_frame_getShortestNameOr(const struct callstack_frame* self,
                                                            const char* fallback) {
    const char* shortest = callstack_frame_getShortestName(self);
    return shortest == NULL ? fallback : shortest;
}

/**
 * Returns the shortest source file name of the given callstack frame.
 *
 * @param self the callstack frame
 * @return the shortest source file name
 * @since v1.1
 */
char * callstack_frame_getShortestSourceFile(const struct callstack_frame * self);

/**
 * @brief Returns the shortest source file name of the given callstack frame.
 *
 * If the given callstack frame does not have a source file name the given
 * fallback is returned.
 *
 * @param self the callstack frame
 * @param fallback the fallback string to be returned
 * @return the shortest source file name or the given fallback
 * @since v1.2
 */
static inline const char* callstack_frame_getShortestSourceFileOr(const struct callstack_frame* self,
                                                                  const char* fallback) {
    const char* shortest = callstack_frame_getShortestSourceFile(self);
    return shortest == NULL ? fallback : shortest;
}

/**
 * Destructs the given callstack frame.
 *
 * @param self the callstack frame to be destructed
 * @since v1.1
 */
void callstack_frame_destroy(const struct callstack_frame* self);

/**
 * Destroys and deallocates the given callstack frame.
 *
 * @param self the callstack frame to be deleted
 * @since v1.1
 */
void callstack_frame_delete(struct callstack_frame* self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __lcs_callstack_frame_h */
