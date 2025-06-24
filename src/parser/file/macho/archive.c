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

#include "archive.h"

#include <ar.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "objectFile.h"
#include "../loader.h"

/**
 * Creates name for the given file name indicating the archive it came from.
 *
 * @param fileName the name of the file out of the archive
 * @param archiveName the name of the archive the file came from
 * @return the allocated name or @c NULL if unable to allocate
 */
static inline char* macho_archive_constructName(const char* fileName, const char* archiveName) {
    if (fileName == NULL || archiveName == NULL) return NULL;
    
    const size_t size = strlen(archiveName) + strlen(fileName) + 3;
    // Why +3: 1 byte for NULL termination and 2 bytes for the parentheses.    - mhahnFr
    
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return NULL;
    }
    strlcpy(toReturn, archiveName, size);
    strlcat(toReturn, "(", size);
    strlcat(toReturn, fileName, size);
    strlcat(toReturn, ")", size);
    toReturn[size - 1] = '\0';
    return toReturn;
}

/**
 * Parses and returns a number in the given base from the given string with the
 * given length.
 *
 * @param string the string
 * @param length the length of the string
 * @param base the base of the number to parse
 * @return the parsed number
 */
static inline size_t macho_archive_parseNumber(const char* string, const size_t length, const int base) {
    char copy[length + 1];
    
    strlcpy(copy, string, length + 1);
    copy[length] = '\0';
    
    return strtoll(copy, NULL, base);
}

/**
 * Calculates and returns the length of the given string without the padding
 * spaces.
 *
 * @param string the string whose length to be determined
 * @param maximumLength the maximum length of the string
 * @return the actual length of the string
 */
static inline size_t macho_archive_stringLength(const char* string, const size_t maximumLength) {
    ssize_t i;
    for (i = (ssize_t) maximumLength - 1; i >= 0 && string[i] == ' '; --i);
    return i + 1;
}

/**
 * Parses the archive in the given buffer.
 *
 * @param buffer the data buffer
 * @param fileName the file name of the archive
 * @param totalSize the total size of the buffer
 * @param cb the callback to be called once an object file has been extracted
 * @return whether the archive was parsed successfully
 */
static inline bool macho_archive_parseImpl(void* buffer, const char* fileName, const size_t totalSize, const macho_archive_callback cb) {
    size_t counter = 0;
    const char* magic = buffer;
    if (strncmp(magic, ARMAG, SARMAG) != 0) {
        return false;
    }
    counter += SARMAG;
    
    const size_t exSize  = strlen(AR_EFMT1),
                 endSize = strlen(ARFMAG);
    while (counter < totalSize) {
        const struct ar_hdr* fileHeader = buffer + counter;
        counter += sizeof(struct ar_hdr);
        if (strncmp(fileHeader->ar_fmag, ARFMAG, endSize) != 0) return false;
        
        char* name;
        size_t nameLength = 0;
        if (strncmp(fileHeader->ar_name, AR_EFMT1, exSize) == 0) {
            const size_t size = macho_archive_parseNumber(fileHeader->ar_name + exSize, sizeof fileHeader->ar_name / sizeof(char) - exSize, 10);
            name = malloc(size + 1);
            if (name == NULL) {
                return false;
            }
            strlcpy(name, buffer + counter, size + 1);
            name[size] = '\0';
            counter += size;
            nameLength = size;
        } else {
            const size_t nameLength = macho_archive_stringLength(fileHeader->ar_name, sizeof fileHeader->ar_name);
            name = malloc(nameLength + 1);
            if (name == NULL) {
                return false;
            }
            strlcpy(name, fileHeader->ar_name, nameLength + 1);
            name[nameLength] = '\0';
        }
        
        void* objectFile = buffer + counter;
        struct objectFile* file = objectFile_new();
        file->lastModified = (time_t) macho_archive_parseNumber(fileHeader->ar_date, sizeof fileHeader->ar_date / sizeof(char), 10);
        file->name = macho_archive_constructName(name, fileName);
        free(name);
        
        file->parsed = objectFile_parseBuffer(file, objectFile);
        cb(file);
        
        counter += macho_archive_parseNumber(fileHeader->ar_size, sizeof fileHeader->ar_size / sizeof(char), 10) - nameLength;
        for (; counter < totalSize && *(char*) (buffer + counter) == '\n'; ++counter);
    }
    return true;
}

bool macho_archive_parse(const char* fileName, const macho_archive_callback cb) {
    return loader_loadFileAndExecute(fileName, (union loader_parserFunction) { 
        .parseFuncExtended = (loader_parserExtended) macho_archive_parseImpl
    }, true, cb);
}
