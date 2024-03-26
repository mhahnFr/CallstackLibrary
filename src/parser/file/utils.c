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

#include "utils.h"

bool loader_loadFileAndExecute(const char* fileName, loader_parser func, void* args) {
    if (fileName == NULL) {
        return false;
    }
    
    struct stat fileStats;
    if (stat(fileName, &fileStats) != 0) {
        return false;
    }
    void* buffer = malloc(fileStats.st_size);
    if (buffer == NULL) {
        return false;
    }
    FILE* file = fopen(fileName, "r");
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    const bool toReturn = (off_t) count == fileStats.st_size && func(args, buffer);
    free(buffer);
    return toReturn;
}
