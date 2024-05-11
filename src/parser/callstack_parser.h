/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef callstack_parser_h
#define callstack_parser_h

#include <callstack.h>
#include <callstack_internals.h>

#include "file/binaryFile.h"

/**
 * The structure of a callstack parser.
 */
struct callstack_parser {
    /** Indicates whether to clear the caches on destruction. */
    bool clearCaches;
};

/**
 * @brief Constructs the given callstack parser object.
 *
 * @param self The callstack parser object to construct.
 */
static inline void callstack_parser_create(struct callstack_parser* self) {
    self->clearCaches = callstack_autoClearCaches;
}

/**
 * @brief Destroys the given callstack parser object.
 *
 * @param self The callstack parser object to destroy.
 */
static inline void callstack_parser_destroy(struct callstack_parser* self) {
    if (self->clearCaches) {
        callstack_clearCaches();
    }
}

/**
 * @brief Parses the debug symbols to create a human readable callstack.
 *
 * Tries to parse the debug symbols, if they are not available, the information of the
 * dynamic linker is used to translate the callstack.
 * If any error occurs, FAILED is returned.
 *
 * @param self The callstack parser object.
 * @param callstack The callstack to parse the symbols for.
 * @return The type of the used symbols.
 */
enum callstack_type callstack_parser_parse(struct callstack_parser * self,
                                           struct callstack * callstack);

/**
 * @brief Demangles the given name if possible and enabled.
 *
 * Either the allocated, demangled name is returned or a copy of the
 * given name.
 *
 * @param name the name to be demangled
 * @return the allocated name
 */
char * callstack_parser_demangle(char * name);

#endif /* callstack_parser_h */
