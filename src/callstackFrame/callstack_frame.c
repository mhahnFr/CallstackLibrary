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

#include <callstack_frame.h>
#include <misc/string_utils.h>

struct callstack_frame* callstack_frame_copy(const struct callstack_frame* self) {
    struct callstack_frame * toReturn = callstack_frame_new();
    
    if (toReturn != NULL) {
        callstack_frame_copyHere(toReturn, self);
    }
    
    return toReturn;
}

void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source) {
    *destination = (struct callstack_frame) {
        source->reserved,
        source->reserved1,
        source->reserved2,
        source->sourceFileOutdated,
        source->binaryFileIsSelf,
        source->reserved1 ? source->binaryFile : utils_maybeCopySave(source->binaryFile, true),
        source->reserved1 ? source->binaryFileRelative : utils_maybeCopySave(source->binaryFileRelative, true),
        source->reserved2 ? source->function : utils_maybeCopySave(source->function, true),
        source->reserved1 ? source->sourceFile : utils_maybeCopySave(source->sourceFile, true),
        source->reserved1 ? source->sourceFileRelative : utils_maybeCopySave(source->sourceFileRelative, true),
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
