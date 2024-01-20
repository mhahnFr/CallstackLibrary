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

struct dwarf_fileNameEntry {
    const char* name;
    
    uint64_t dirIndex;
    uint64_t modTime;
    uint64_t size;
};

typedef_vector_light_named(dwarfFileEntry, struct dwarf_fileNameEntry);

#endif /* fileNameEntry_h */
