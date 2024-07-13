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

#ifndef fileAttribute_h
#define fileAttribute_h

#include <stdint.h>

/**
 * @brief This structure represents a DWARF 5 file attribute.
 *
 * Unavailable fields are set to `0` and `NULL`, respectively.
 */
struct fileAttribute {
    /** The path name.                          */
    char*    path;
    /** The directory index.                    */
    uint64_t index;
    /** The timestamp of the last modification. */
    uint64_t timestamp;
    /** The size of the file entity.            */
    uint64_t size;
    /** The MD5 hash of the file entity.        */
    uint8_t* md5;
};

#endif /* fileAttribute_h */
