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

#ifndef macho_parser_nlist_h
#define macho_parser_nlist_h

#include <stdbool.h>
#include <stdint.h>

#include <mach-o/nlist.h>

#include <macho/macho_utils.h>

/**
 * Represents a unified version of the `nlist` structure.
 */
struct macho_parser_nlist {
    /** The index into the string table.           */
    uint32_t n_strx;
    /** The type of this entry.                    */
    uint8_t  n_type;
    /** The index of the section the symbol is in. */
    uint8_t  n_sect;
    /** Additional description of the symbol.      */
    int32_t  n_desc;
    /** The value (payload).                       */
    uint64_t n_value;
};

/**
 * Constructs a unified nlist entry from the given information.
 *
 * @param pointer the pointer to the real nlist entry
 * @param bit64 whether the binary is 64 bit encoded
 * @param bytesSwapped whether the byte order needs to be swapped to match the host byte order
 * @return the translated and unified nlist entry
 */
static inline struct macho_parser_nlist macho_parser_nlist_from(const void* pointer, bool bit64, bool bytesSwapped) {
    struct macho_parser_nlist toReturn;
    if (bit64) {
        const struct nlist_64* real = pointer;

        toReturn = (struct macho_parser_nlist) {
            .n_strx  = macho_maybeSwap(32, bytesSwapped, real->n_un.n_strx),
            .n_type  = real->n_type,
            .n_sect  = real->n_sect,
            .n_desc  = macho_maybeSwap(16, bytesSwapped, real->n_desc),
            .n_value = macho_maybeSwap(64, bytesSwapped, real->n_value)
        };
    } else {
        const struct nlist* real = pointer;
        
        toReturn = (struct macho_parser_nlist) {
            .n_strx  = macho_maybeSwap(32, bytesSwapped, real->n_un.n_strx),
            .n_type  = real->n_type,
            .n_sect  = real->n_sect,
            .n_desc  = macho_maybeSwap(16, bytesSwapped, real->n_desc),
            .n_value = macho_maybeSwap(32, bytesSwapped, real->n_value)
        };
    }
    return toReturn;
}

/**
 * Returns the size of the real nlist structure used.
 *
 * @param bit64 whether the 64 bit version is used
 * @return the size in bytes of the real nöist structure used
 */
static inline size_t macho_parser_nlist_sizeof(bool bit64) {
    return bit64 ? sizeof(struct nlist_64) : sizeof(struct nlist);
}

#endif /* macho_parser_nlist_h */
