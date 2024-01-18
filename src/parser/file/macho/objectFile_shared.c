/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "objectFile.h"

static inline bool objectFile_parseDwarf(struct objectFile* self, void* buffer, void (*callback)(void)) {
    return false;
}

bool objectFile_parse(struct objectFile* self, void (*callback)(void)) {
    if (self == NULL) return false;
    
    struct stat fileStats;
    if (stat(self->name, &fileStats) != 0) {
        return false;
    }
    void* buffer = malloc(fileStats.st_size);
    if (buffer == NULL) {
        return false;
    }
    FILE* file = fopen(self->name, "r");
    if (file == NULL) {
        free(buffer);
        return false;
    }
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    const bool success = (off_t) count == fileStats.st_size && objectFile_parseDwarf(self, buffer, callback);
    free(buffer);
    return success;
}
