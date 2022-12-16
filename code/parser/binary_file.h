/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022  mhahnFr
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

#ifndef binary_file_h
#define binary_file_h

#include "dwarf/dwarf_file.h"
#include "llvm/llvm_file.h"

#include <stddef.h>

/**
 * The different types supported by the binary file abstraction.
 */
enum binary_file_type {
    /** A DWARF file.            */
    DWARF,
    /** A LLVM file.             */
    LLVM,
    /** The type is not defined. */
    UNDEFINED
};

/**
 * The binary_file serves as an abstraction of the DWARF and the LLVM binary files.
 */
struct binary_file {
    /** The file type.              */
    enum binary_file_type fileType;
    /** The name of the file.       */
    char * fileName;
    /** The underlying binary file. */
    void * parsedBinary;
};

/**
 * @brief Constructs the given binary_file.
 *
 * All values are set as empty.
 *
 * @param self The binary_file to be constructed.
 */
static inline void binary_file_create(struct binary_file * self) {
    self->fileType     = UNDEFINED;
    self->fileName     = NULL;
    self->parsedBinary = NULL;
}

/**
 * @brief Constructs the given binary_file using the given dwarf_file.
 *
 * @param self The binary_file to be constructed.
 * @param fileName The file name.
 * @param dwarf The dwarf_file.
 */
static inline void binary_file_create_from_dwarf(struct binary_file * self,
                                                 char *               fileName,
                                                 struct dwarf_file *  dwarf) {
    self->fileType     = DWARF;
    self->fileName     = fileName;
    self->parsedBinary = dwarf;
}

/**
 * @brief Constructs the given binary_file using the given llvm_file.
 *
 * @param self The binary_file to be constructed.
 * @param fileName The name of the file.
 * @param llvm The llvm_file.
 */
static inline void binary_file_create_from_llvm(struct binary_file * self,
                                                char *               fileName,
                                                struct llvm_file *   llvm) {
    self->fileType     = LLVM;
    self->fileName     = fileName;
    self->parsedBinary = llvm;
}

/**
 * @brief Constructs the given binary_file by allocating a new dwarf_file.
 *
 * @param self The binary_file to be constructed.
 * @param fileName The name of the file.
 */
static inline void binary_file_create_dwarf(struct binary_file * self,
                                            char *               fileName) {
    self->fileType     = DWARF;
    self->fileName     = fileName;
    self->parsedBinary = dwarf_file_new(fileName);
}

/**
 * @brief Constructs the given binary_file by allocating a new llvm_file.
 *
 * @param self The binary_file to be constructed.
 * @param fileName The name of the file.
 */
static inline void binary_file_create_llvm(struct binary_file * self,
                                           char *               fileName) {
    self->fileType     = LLVM;
    self->fileName     = fileName;
    self->parsedBinary = llvm_file_new(fileName);
}

/**
 * @brief Destroys the given binary_file.
 *
 * The underlying DWARF or LLVM file is deleted.
 *
 * @param self The binary_file to be destroyed.
 */
static inline void binary_file_destroy(struct binary_file * self) {
    switch (self->fileType) {
        case DWARF: dwarf_file_delete(self->parsedBinary); break;
        case LLVM:  llvm_file_delete(self->parsedBinary); break;
            
        default: break;
    }
}

#endif /* binary_file_h */
