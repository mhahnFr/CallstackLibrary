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

#include <stddef.h>
#include <stdint.h>

#include "dwarf_parser.h"

static inline bool dwarf_parseLineProgramV4(void* begin, size_t counter, uint64_t actualSize, dwarf_line_callback cb) {
    // TODO: Imeplement
    
    (void) begin;
    (void) counter;
    (void) actualSize;
    (void) cb;
    
    return true;
}

static inline bool dwarf_parseLineProgramV4_64(void* begin, size_t counter, uint64_t actualSize, dwarf_line_callback cb) {
    // TODO: Implement
    
    (void) begin;
    (void) counter;
    (void) actualSize;
    (void) cb;
    
    return true;
}

bool dwarf_parseLineProgram(void* begin, dwarf_line_callback cb) {
    size_t counter = 0;
    
    const uint32_t size = *((uint32_t*) begin);
    counter += 4;
    
    bool     bit64;
    uint64_t actualSize;
    if (size == 0xffffffff) {
        actualSize = *((uint64_t*) (begin + counter));
        bit64      = true;
        counter += 8;
    } else {
        actualSize = size;
        bit64      = false;
    }
    const uint16_t version = *((uint16_t*) (begin + counter));
    counter += 2;
    
    if (bit64) {
        switch (version) {
            case 4: return dwarf_parseLineProgramV4_64(begin, counter, actualSize, cb);
                
            default: return false;
        }
    }
    switch (version) {
        case 4: return dwarf_parseLineProgramV4(begin, counter, actualSize, cb);
            
        default: return false;
    }
    return false;
}
