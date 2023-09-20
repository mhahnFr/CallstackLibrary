/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#ifndef callstack_parserInternal_h
#define callstack_parserInternal_h

#include "callstack_parser.h"

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
 * @brief Creates a callstack line using the given callstack, info, index and fallback.
 *
 * The callstack line of the given index is translated using the given dynamic linker information.
 * If translating is not possible, the fallback is used to create the callstack line.
 *
 * @param callstack the callstack whose line to be translated
 * @param info the dynamic linker info
 * @param index the index of the desired callstack line
 * @param fallback the fallback string
 * @return whether the line was set
 */
bool callstack_parser_createDynamicLine(struct callstack * callstack,
                                               Dl_info *   info,
                                               size_t      index,
                                               char *      fallback);

#endif /* callstack_parserInternal_h */