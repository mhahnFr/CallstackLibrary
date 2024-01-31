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

#ifndef fileNameEntry_h
#define fileNameEntry_h

#include <stdint.h>

#include "../../../../DC4C/vector.h"

/**
 * This structure represents a line entry in the DWARF line program.
 */
struct dwarf_fileNameEntry {
    /** The file name this entry is in.             */
    const char* name;
    
    /** The index of the directory the file was in. */
    uint64_t dirIndex;
    /** The last modified time stamp of the file.   */
    uint64_t modTime;
    /** The size in bytes of the file.              */
    uint64_t size;
};

typedef_vector_light_named(dwarfFileEntry, struct dwarf_fileNameEntry);

#endif /* fileNameEntry_h */
