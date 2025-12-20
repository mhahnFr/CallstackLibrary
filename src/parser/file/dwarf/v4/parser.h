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

#ifndef dwarf_v4_parser_h
#define dwarf_v4_parser_h

#include <misc/string_utils.h>

#include "fileNameEntry.h"

struct dwarf_parser;

/**
 * This structure represents the specifics for DWARF in version 4 or earlier.
 */
struct dwarf4_parser {
    /** The include directories searched while compiling a source file. */
    vector_string_t includeDirectories;
    /** The names of the files that contributed to the line numbers.    */
    vector_dwarfFileEntry_t fileNames;
};

/**
 * Constructs the specific part of the given DWARF parser for version 4 and earlier.
 *
 * @param self the generified parser object
 */
void dwarf4_parser_create(struct dwarf_parser* self);

#endif /* dwarf_v4_parser_h */
