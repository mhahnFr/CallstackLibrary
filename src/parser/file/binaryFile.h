/*
 * Callstack Library - Library creating human-readable call stacks.
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

#ifndef binaryFile_h
#define binaryFile_h

#include <stdbool.h>

#include "../../../include/callstack_frame.h"
#include "../../../include/lcs_dlfcn.h"

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
    /** The type of this binary file.                        */
    enum binaryFileType type;
    /** A pointer to the concrete structure.                 */
    void * concrete;
    
    /** Indicates whether this file has already been parsed. */
    bool parsed;
    
    /** The name of the represented binary file.             */
    const char * fileName;
    
    /** A pointer to the next binary file structure.         */
    struct binaryFile * next;
    
    /** Translating method, heavily WIP.                     */
    bool (*addr2String)(struct binaryFile *, Dl_info *, void *, struct callstack_frame *);
    /** The appropriate deinitializing method.               */
    void (*destroy)    (struct binaryFile *);
    /** The appropriate deleting method.                     */
    void (*delete)     (struct binaryFile *);
};

/**
 * Allocates a new concrete binary file structure.
 *
 * @param fileName the name of the represented binary file
 */
struct binaryFile * binaryFile_new(const char * fileName);

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
char * binaryFile_toAbsolutePath(char * path);
char * binaryFile_toAbsolutePathFree(char * path);

#endif /* binaryFile_h */
