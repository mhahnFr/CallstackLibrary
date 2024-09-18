/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef dwarf_v5_parser_h
#define dwarf_v5_parser_h

#include <stdbool.h>
#include <stdint.h>

#include "vector_fileAttribute.h"

#include "../../lcs_section.h"

struct dwarf_parser;

/**
 * This structure represents the specific parser part for DWARF in version 5.
 */
struct dwarf5_parser {
    /** The file attributes of the include directories.                      */
    vector_fileAttribute_t directories,
    /** The file attributes of the files that contributed to the line table. */
                           files;
};

/**
 * Constructs the specific part of the given generified DWARF parser for version 5.
 *
 * @param self the generified parser object
 */
void dwarf5_parser_create(struct dwarf_parser* self);

/**
 * Consumes the following data block of different possible types, according to the
 * formats available for additional vendor specific data.
 *
 * @param self the dwarf parser structure
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the expected data type
 * @return whether the data was allowed and skipped successfully
 */
bool dwarf5_consumeSome(struct dwarf_parser* self, void* buffer, size_t* counter, uint64_t type);

/**
 * @brief Reads a string.
 *
 * The string may follow in the given data buffer or may come from one of the debug string sections.
 * The returned string is not allocated.
 *
 * @param self the dwarf parser structure
 * @param buffer the data buffer
 * @param counter the reading index into the given data buffer
 * @param type the type of string to load
 * @return a pointer to the string which points into either the given data buffer or into one of the given sections;
 * `NULL` is returned if the given data type was not allowed
 */
char* dwarf5_readString(struct dwarf_parser* self, void* buffer, size_t* counter, uint64_t type);

#endif /* dwarf_v5_parser_h */
