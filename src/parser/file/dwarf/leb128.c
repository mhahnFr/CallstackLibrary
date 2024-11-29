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

#include <stdbool.h>

#include "leb128.h"

uint64_t getULEB128(void* begin, size_t* counter) {
    uint64_t result = 0,
             shift  = 0;

    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if (b < 0x80) {
            more = false;
        }
    } while (more);
    return result;
}

int64_t getLEB128(void* begin, size_t* counter) {
    int64_t result = 0,
            shift  = 0;

    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if ((0x80 & b) == 0) {
            if (shift < 32 && (b & 0x40) != 0) {
                result |= ((uint64_t) ~0 << shift);
            }
            more = false;
        }
    } while (more);
    return result;
}
