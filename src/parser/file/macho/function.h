/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#ifndef function_h
#define function_h

#include <stdint.h>

/**
 * This structure represents a function in a Mach-O symbol table.
 */
struct function {
    /** The beginning address of the function inside its Mach-O file. */
    uint64_t startAddress;
    /** The end address of the function inside its Mach-O file.       */
    uint64_t endAddress;
    
    /** The name of the function at linking time.                     */
    char * linkedName;
};

/**
 * Allocates a new function structure.
 *
 * @return the allocated function structure or `NULL` on error
 */
struct function * function_new(void);

/**
 * Initializes the given function structure.
 *
 * @param self the function structure to be initialized
 */
void function_create(struct function * self);

/**
 * Deinitializes the given function structure.
 *
 * @param self the function structure to be deinitialized
 */
void function_destroy(struct function * self);

/**
 * Deinitializes and `free`s the given function structure.
 *
 * @param self the function structure to be deleted
 */
void function_delete(struct function * self);

#endif /* function_h */
