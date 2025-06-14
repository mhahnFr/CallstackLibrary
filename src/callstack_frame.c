/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#include <string.h>

#include <callstack_frame.h>

struct callstack_frame* callstack_frame_copy(const struct callstack_frame* self) {
    struct callstack_frame * toReturn = callstack_frame_new();
    
    if (toReturn != NULL) {
        callstack_frame_copyHere(toReturn, self);
    }
    
    return toReturn;
}

/**
 * Creates a copy of the given string if it is not `NULL`.
 *
 * @param str the string to be copied
 * @return the copy or `NULL` if `NULL` was passed or unable to allocate
 */
static inline char * maybeStrdup(const char * str) {
    return str == NULL ? NULL : strdup(str);
}

void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source) {
    *destination = (struct callstack_frame) {
        source->reserved,
        source->reserved1,
        source->reserved2,
        source->reserved1 ? source->binaryFile : maybeStrdup(source->binaryFile),
        source->reserved1 ? source->binaryFileRelative : maybeStrdup(source->binaryFileRelative),
        source->reserved2 ? source->function : maybeStrdup(source->function),
        source->reserved1 ? source->sourceFile : maybeStrdup(source->sourceFile),
        source->reserved1 ? source->sourceFileRelative : maybeStrdup(source->sourceFileRelative),
        source->sourceFileOutdated,
        source->binaryFileIsSelf,
        source->sourceLine,
        source->sourceLineColumn
    };
}

char * callstack_frame_getShortestName(const struct callstack_frame * self) {
    if (self->binaryFileRelative == NULL) {
        return self->binaryFile;
    } else if (self->binaryFile == NULL) {
        return self->binaryFileRelative;
    }
    
    const size_t s1 = strlen(self->binaryFile),
                 s2 = strlen(self->binaryFileRelative);
    return s2 < s1 ? self->binaryFileRelative : self->binaryFile;
}

char * callstack_frame_getShortestSourceFile(const struct callstack_frame * self) {
    if (self->sourceFileRelative == NULL) {
        return self->sourceFile;
    } else if (self->sourceFile == NULL) {
        return self->sourceFileRelative;
    }
    
    const size_t s1 = strlen(self->sourceFile),
                 s2 = strlen(self->sourceFileRelative);
    return s2 < s1 ? self->sourceFileRelative : self->sourceFile;
}
