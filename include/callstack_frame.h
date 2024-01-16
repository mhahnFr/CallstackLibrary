/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef callstack_frame_h
#define callstack_frame_h

#include "optional_Dl_info.h"
#include "optional_ulong.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/**
 * This structure represents a translated callstack frame.
 */
struct callstack_frame {
    /** The dynamic loader info if available.                */
    optional_Dl_info_t info;
    
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
    optional_ulong_t sourceLineColumn;
};

/**
 * Constructs the given callstack frame.
 *
 * @param self the callstack frame to be initialized
 */
static inline void callstack_frame_create(struct callstack_frame * self) {
    self->binaryFile         = NULL;
    self->binaryFileRelative = NULL;
    self->function           = NULL;
    self->sourceFile         = NULL;
    self->sourceFileRelative = NULL;
    self->sourceLine         = 0;
    
    self->sourceLineColumn.has_value = false;
    
    self->info.has_value = false;
}

/**
 * Allocates a new and initialized callstack frame.
 *
 * @return the allocated callstack frame or `NULL` if unable to allocate
 */
static inline struct callstack_frame * callstack_frame_new(void) {
    struct callstack_frame * toReturn = (struct callstack_frame *) malloc(sizeof(struct callstack_frame));
    
    if (toReturn != NULL) {
        callstack_frame_create(toReturn);
    }
    
    return toReturn;
}

/**
 * Allocates a new callstack frame and deeply copies the given callstack frame.
 *
 * @param self the callstack frame to be copied
 * @return a copy of the given callstack frame or `NULL` if unable to allocate
 */
struct callstack_frame * callstack_frame_copy(struct callstack_frame * self);

/**
 * Copies the given callstack frame into the given destination.
 *
 * @param destination the callstack frame filled with the copy
 * @param source the callstack frame to be copied
 */
void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source);

/**
 * Returns the shortest binary file name of the given callstack frame.
 *
 * @param self the callstack frame
 * @return the shortest name
 */
char * callstack_frame_getShortestName(const struct callstack_frame * self);

/**
 * Returns the shortest source file name of the given callstack frame.
 *
 * @param self the callstack frame
 * @return the shortest source file name
 */
char * callstack_frame_getShortestSourceFile(const struct callstack_frame * self);

/**
 * Destructs the given callstack frame.
 *
 * @param self the callstack frame to be destructed
 */
static inline void callstack_frame_destroy(struct callstack_frame * self) {
    free(self->binaryFile);
    free(self->binaryFileRelative);
    free(self->function);
    free(self->sourceFile);
    free(self->sourceFileRelative);
}

/**
 * Destroys and deallocates the given callstack frame.
 *
 * @param self the callstack frame to be deleted
 */
static inline void callstack_frame_delete(struct callstack_frame * self) {
    callstack_frame_destroy(self);
    free(self);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_frame_h */
