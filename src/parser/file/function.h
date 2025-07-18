/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#ifndef function_h
#define function_h

#include <stdint.h>

#include <DC4C/optional.h>

#include "optional_string.h"

/**
 * This structure represents a function in a Mach-O symbol table.
 */
struct function {
    /** The beginning address of the function inside its Mach-O file. */
    uint64_t startAddress;
    /** The length of this function.                                  */
    uint64_t length;
    
    /** The name of the function at linking time.                     */
    char * linkedName;
    /** The demangled name of the function.                           */
    optional_string_t demangledName;
};

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
void function_destroy(const struct function* self);

typedef_optional_named(function, struct function);

#endif /* function_h */
