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

#ifndef binaryFile_h
#define binaryFile_h

#define _GNU_SOURCE
 #define __USE_GNU
  #include <dlfcn.h>
 #undef __USE_GNU
#undef _GNU_SOURCE

#include <stdbool.h>

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
    char * (*addr2String)(struct binaryFile *, Dl_info *, void *);
    /** The appropriate deinitializing method.               */
    void   (*destroy)    (struct binaryFile *);
    /** The appropriate deleting method.                     */
    void   (*delete)     (struct binaryFile *);
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

#endif /* binaryFile_h */
