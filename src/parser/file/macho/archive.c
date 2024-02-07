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

#include <ar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "archive.h"
#include "objectFile.h"

static inline char* macho_archive_constructName(const char* fileName, const char* archiveName) {
    if (fileName == NULL || archiveName == NULL) return NULL;
    
    const size_t size = strlen(archiveName) + strlen(fileName) + 3;
    // Why +3: 1 byte for NUL termination and 2 bytes for the parentheses.    - mhahnFr
    
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return NULL;
    }
    strlcpy(toReturn, archiveName, size);
    strlcat(toReturn, "(", size);
    strlcat(toReturn, fileName, size);
    strlcat(toReturn, ")", size);
    return toReturn;
}

static inline bool macho_archive_parseImpl(void* buffer, const char* fileName, const size_t totalSize, macho_archive_callback cb) {
    size_t counter = 0;
    const char* magic = buffer;
    if (strncmp(magic, ARMAG, SARMAG) != 0) {
        return false;
    }
    counter += SARMAG;
    
    const size_t exSize = strlen(AR_EFMT1);
    while (counter < totalSize) {
        struct ar_hdr* fileHeader = buffer + counter;
        counter += sizeof(struct ar_hdr);
        
        char* name;
        size_t nameLength = 0;
        if (strncmp(fileHeader->ar_name, AR_EFMT1, exSize) == 0) {
            const size_t size = strtoll(fileHeader->ar_name + exSize, NULL, 10); // FIXME: Length!
            name = malloc(size + 1); // TODO: Abort parsing, but what's with the already parsed object files?
            strlcpy(name, buffer + counter, size + 1);
            counter += size;
            nameLength = size;
        } else {
            // TODO: Gather the correct length
            name = malloc(17); // TODO: Abort parsing, but what's with the already parsed object files?
            strlcpy(name, fileHeader->ar_name, 17);
        }
        
        void* objectFile = buffer + counter;
        struct objectFile* file = objectFile_new();
        file->lastModified = strtoll(fileHeader->ar_date, NULL, 10); // FIXME: Length!
        file->name = macho_archive_constructName(name, fileName);
        free(name);
        
        file->parsed = objectFile_parseBuffer(file, objectFile);
        cb(file);
        
        counter += strtoll(fileHeader->ar_size, NULL, 10) - nameLength; // FIXME: Length!
        for (; counter < totalSize && *((char*) (buffer + counter)) == '\n'; ++counter);
    }
    return true;
}

bool macho_archive_parse(const char* fileName, macho_archive_callback cb) {
    if (fileName == NULL) return false;
    
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
    const bool toReturn = (off_t) count == fileStats.st_size && macho_archive_parseImpl(buffer, fileName, count, cb);
    free(buffer);
    return toReturn;
}
