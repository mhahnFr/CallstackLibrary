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

#ifndef binaryFile_h
#define binaryFile_h

#include <stdbool.h>

#include "../../../include/callstack_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration contains the supported types of executable files.
 */
enum binaryFileType {
    /** Represents a Mach-O binary file. */
    MACHO_FILE,
    /** Represents an ELF binary file.   */
    ELF_FILE
};

/**
 * This structure represents a generic binary executable file.
 */
struct binaryFile {
    /** The type of this binary file.                               */
    enum binaryFileType type;
    /** A pointer to the concrete structure.                        */
    void * concrete;
    
    /** Indicates whether this file has already been parsed.        */
    bool parsed;
    
    /** The name of the represented binary file.                    */
    const char * fileName;
    
    /** The start address in memory of the represented binary file. */
    void* startAddress;
    
    /** A pointer to the next binary file structure.                */
    struct binaryFile * next;
    
    /**
     * @brief The translating method.
     *
     * Attempts to translate the given address into the given callstack
     * frame object.
     *
     * Returns whether the address could be translated.
     */
    bool (*addr2String)(struct binaryFile*, void*, struct callstack_frame*);
    /** The appropriate deinitializing method.                      */
    void (*destroy)    (struct binaryFile *);
    /** The appropriate deleting method.                            */
    void (*deleter)    (struct binaryFile *);
};

/**
 * Allocates a new concrete binary file structure.
 *
 * @param fileName the name of the represented binary file
 * @param startAddress the start address of the represented binary file
 */
struct binaryFile* binaryFile_new(const char* fileName, void* startAddress);

/**
 * Initializes the given binary file structure.
 *
 * @param self the binary file structure to be initialized
 */
void binaryFile_create(struct binaryFile * self);

/**
 * @brief Converts the given path to a relative path.
 *
 * If `CXX_FUNCTIONS` is not defined, the given path is only duplicated.
 *
 * @param path the path to create a relative one from
 * @return a newly allocated relative path
 */
char * binaryFile_toRelativePath(char * path);
/**
 * @brief Converts the given path to a relative path and frees the given path.
 *
 * If `CXX_FUNCTIONS` is not defined, the given path is only duplicated.
 *
 * @param path the path to create a relative one from
 * @return a newly allocated relative path
 */
char * binaryFile_toRelativePathFree(char * path);
/**
 * @brief Converts the given path to an absolute path.
 *
 * If `CXX_FUNCTIONS` is not defined, the given path is only duplicated.
 *
 * @param path the path to create an absolute one from
 * @return a newly allocated absolute path
 */
char * binaryFile_toAbsolutePath(char * path);
/**
 * @brief Converts the given path to an absolute path and frees the given path.
 *
 * If `CXX_FUNCTIONS` is not defined, the given path is only duplicated.
 *
 * @param path the path to create an absolute one from
 * @return a newly allocated absolute path
 */
char * binaryFile_toAbsolutePathFree(char * path);

/**
 * @brief Finds the appropriate binary file object in the cache.
 *
 * If no object was found, it is created and added to the cache.
 *
 * @param fileName the name of the file
 * @param startAddress the start address
 * @return the binary file object or `NULL` if unable to allocate
 */
struct binaryFile* binaryFile_findOrAddFile(const char* fileName, void* startAddress);

/**
 * Clears the caches created by the binary file implementations.
 */
void binaryFile_clearCaches(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* binaryFile_h */
