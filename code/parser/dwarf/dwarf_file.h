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

#ifndef dwarf_file_h
#define dwarf_file_h

#include <stdlib.h>

/**
 * Represents a binary file using DWARF debug symbols.
 */
struct dwarf_file {
    /** The file name. */
    char * fileName;
    // ...
};

/**
 * @brief Allocates a new dwarf_file.
 *
 * It is constructed using the given file name.
 *
 * @return An allocated dwarf_file.
 */
struct dwarf_file * dwarf_file_new(char * fileName);

/**
 * @brief Constructs the given dwarf_file.
 *
 * @param self The dwarf_file to be constructed.
 * @param fileName The name of the file.
 */
static inline void dwarf_file_create(struct dwarf_file * self, char * fileName) {
    self->fileName = fileName;
}

/**
 * @brief Destroys the given dwarf_file.
 *
 * @param self The dwarf_file to be destroyed.
 */
static inline void dwarf_file_destroy(struct dwarf_file * self) {
    (void) self;
}

/**
 * @brief Deletes the given dwarf_file.
 *
 * It is destroyed before it is deallocated.
 *
 * @param self The dwarf_file to be deleted.
 */
static inline void dwarf_file_delete(struct dwarf_file * self) {
    dwarf_file_destroy(self);
    free(self);
}

#endif /* dwarf_file_h */
