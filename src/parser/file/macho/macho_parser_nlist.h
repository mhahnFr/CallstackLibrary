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

#ifndef macho_parser_nlist_h
#define macho_parser_nlist_h

#include <stdbool.h>
#include <stdint.h>

#include <mach-o/nlist.h>

#include "macho_utils.h"

struct macho_parser_nlist {
    uint32_t n_strx;
    uint8_t  n_type;
    uint8_t  n_sect;
    int32_t  n_desc;
    uint64_t n_value;
};

static inline struct macho_parser_nlist macho_parser_nlist_from(void* pointer, bool bit64, bool bytesSwapped) {
    struct macho_parser_nlist toReturn;
    if (bit64) {
        struct nlist_64* real = pointer;
        
        toReturn = (struct macho_parser_nlist) {
            .n_strx  = macho_maybeSwap(32, bytesSwapped, real->n_un.n_strx),
            .n_type  = real->n_type,
            .n_sect  = real->n_sect,
            .n_desc  = macho_maybeSwap(16, bytesSwapped, real->n_desc),
            .n_value = macho_maybeSwap(64, bytesSwapped, real->n_value)
        };
    } else {
        struct nlist* real = pointer;
        
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

static inline size_t macho_parser_nlist_sizeof(bool bit64) {
    return bit64 ? sizeof(struct nlist_64) : sizeof(struct nlist);
}

#endif /* macho_parser_nlist_h */
