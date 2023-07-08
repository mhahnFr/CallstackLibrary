/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#include "binaryFile.h"

#include "macho/machoFile.h"

struct binaryFile * binaryFile_new(const char * fileName) {
    struct binaryFile * toReturn;
    
    // TODO: ifdef macOS -> machoFile, elifdef Linux -> elfFile
    toReturn = &machoFile_new()->_;
    // toReturn = &elfFile_new()->_;
    
    if (toReturn != NULL) {
        toReturn->fileName = fileName;
    }
    return toReturn;
}

void binaryFile_create(struct binaryFile * self) {
    self->fileName = NULL;
    self->next     = NULL;
    self->parsed   = false;
}
