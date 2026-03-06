/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include "loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "exception.h"

void loader_loadFileAndExecuteTime(const char* fileName, const time_t* lastModified,
                                   const union loader_parserFunction func, const bool extended, void* args) {
    if (fileName == NULL) {
        BFE_THROW_RAW(empty, fileName, "No file name given");
    }
    struct stat fileStats;
    if (stat(fileName, &fileStats) != 0) {
        BFE_THROW_RAW(failed, fileName, "Could not stat file name");
    }
    if (lastModified != NULL && fileStats.st_mtime != *lastModified) {
        BFE_THROW_RAW(invalid, fileName, "File last modified timestamp does not match given last modified time");
    }
    void* buffer = BFE_ALLOC_MSG(fileStats.st_size, "Failed to allocate file buffer");
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        free(buffer);
        BFE_THROW_RAW(failed, fileName, "Could not open file");
    }
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    if ((off_t) count != fileStats.st_size) {
        free(buffer);
        BFE_THROW_RAW(failed, fileName, "Could not read file");
    }
    TRY({
        if (extended) {
            func.parseFuncExtended(buffer, fileName, count, args);
        } else {
            func.parseFunc(args, buffer);
        }
    }, CATCH_ALL(_, {
        (void) _;
        free(buffer);
        RETHROW;
    }))
    free(buffer);
}
