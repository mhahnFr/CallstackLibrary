/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef loadedLibInfo_h
#define loadedLibInfo_h

#include <stdbool.h>

#include "parser/file/binaryFile.h"

/**
 * This structure represents a loaded runtime library.
 */
struct loadedLibInfo {
    /** The start address of the runtime image.                           */
    const void* begin,
    /** The end address of the runtime image.                             */
              * end;

    /** The relocation offset of the represented binary file.             */
    uintptr_t relocationOffset;

    /** The file name of the loaded runtime image as given by the system. */
    char* fileName;
    /** The generated absolute file name of the runtime image.            */
    char* absoluteFileName;
    /** The generated relative file name of the runtime image.            */
    char* relativeFileName;

    /** Indicates whether the runtime image is ours.                      */
    bool isSelf;

    /** The associated binary file abstraction object.                    */
    struct binaryFile* associated;
};

/**
 * The initializing values for the @c loadedLibInfo .
 */
#define loadedLibInfo_initializer ((struct loadedLibInfo) { NULL, NULL, 0, NULL, NULL, NULL, false, NULL })

/**
 * Prepares the associated binary file.
 *
 * @param self the loaded library information to prepare
 * @return whether the preparation was successful
 */
bool loadedLibInfo_prepare(struct loadedLibInfo* self);

/**
 * Destroys the given library info.
 *
 * @param self the library info to be destroyed
 */
void loadedLibInfo_destroy(const struct loadedLibInfo* self);

#endif /* loadedLibInfo_h */
