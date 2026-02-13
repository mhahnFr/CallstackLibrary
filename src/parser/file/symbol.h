/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#ifndef symbol_h
#define symbol_h

#include <stdint.h>
#include <DC4C/optional.h>
#include <DC4C/vector.h>
#include <misc/string_utils.h>

/**
 * This structure represents a symbol entry in a symbol table.
 */
struct symbol {
    /** The beginning address of the symbol inside its binary file. */
    uint64_t startAddress;
    /** The length of this symbol.                                  */
    uint64_t length;
    
    /** The name of the symbol at linking time.                     */
    char * linkedName;
    /** The demangled name of the symbol.                           */
    optional_string_t demangledName;
};

/**
 * The initializing expression of the symbol structure.
 */
#define symbol_initializer (struct symbol) { 0, 0, NULL, { .has_value = false }}

/**
 * Deinitializes the given symbol structure.
 *
 * @param self the symbol structure to be deinitialized
 */
void symbol_destroy(const struct symbol* self);

typedef_optional_named(symbol, struct symbol);
typedef_vector_named(symbol, struct symbol);

#endif /* symbol_h */
