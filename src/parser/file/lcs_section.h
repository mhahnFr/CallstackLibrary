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

#ifndef lcs_section_h
#define lcs_section_h

#include <stdint.h>

/**
 * This structure represents a section inside a binary file.
 */
struct lcs_section {
    /** Pointer to the actual content. */
    void* content;
    /** The size of the content.       */
    uint64_t size;
};

/**
 * Initializes the given section structure.
 *
 * @param self the section structure object to be initialized
 */
static inline void lcs_section_create(struct lcs_section* self) {
    *self = (struct lcs_section) { NULL, 0 };
}

#endif /* lcs_section_h */
