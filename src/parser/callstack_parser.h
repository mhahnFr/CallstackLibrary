/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef callstack_parser_h
#define callstack_parser_h

#include "../../include/callstack.h"
#include "../../include/callstack_internals.h"
#include "file/binaryFile.h"

/**
 * The structure of a callstack parser.
 */
struct callstack_parser {
    /** A list of the already parsed files. */
    struct binaryFile * parsedFiles;
};

/**
 * @brief Constructs the given callstack parser object.
 *
 * @param self The callstack parser object to construct.
 */
static inline void callstack_parser_create(struct callstack_parser * self) {
    self->parsedFiles = NULL;
}

/**
 * @brief Destroys the given callstack parser object.
 *
 * @param self The callstack parser object to destroy.
 */
void callstack_parser_destroy(struct callstack_parser * self);

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
 * Returns the appropriate cache for the given parser structure.
 *
 * @param self the parser structure
 * @return the appropriate cache
 */
static inline struct binaryFile ** callstack_parser_getCache(struct callstack_parser * self) {
    return callstack_autoClearCaches ? &self->parsedFiles : NULL;
}

/**
 * @brief Translates the given callstack using the given parser.
 *
 * @param self the callstack parser to be used
 * @param callstack the callstack to be translated
 * @return whether the translation was successful
 */
bool callstack_parser_parseImpl(struct callstack_parser * self,
                                struct callstack *        callstack);

/**
 * @brief Creates a callstack line, demangling the given name if possible and enabled.
 *
 * An allocated callstack line is returned.
 *
 * @param name the name to be tried to demangled
 * @param diff the function depth
 * @return the callstack line or `NULL` on error
 */
char * callstack_parser_createLine(const char * name, ptrdiff_t diff);

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

/**
 * @brief Creates a callstack line using the given info and frame address.
 *
 * The given frame address is translated using the given dynamic linker information.
 *
 * @param info the dynamic linker info
 * @param frameAddress the translated frame's address
 * @param frame the frame structure to store the information in
 * @return whether the line could be set
 */
bool callstack_parser_createDynamicLine(Dl_info *         info,
                                        void *            frameAddress,
                                struct callstack_frame * frame);

#endif /* callstack_parser_h */
