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

bool callstack_parser_parseImpl(struct callstack_parser * self,
                                struct callstack *        callstack);

/**
 * @brief If the demangling is enabled, tries to demangle the given name.
 *
 * If the name could not be demangled, a copy of it is returned. Otherwise, an allocated,
 * demangled string is returned.
 *
 * @param name The name to be tried to demangle.
 * @param diff The difference between the current address and the return address.
 * @return A copy of the given name or the demangled name.
 */
char * callstack_parser_demangle(const char * name, ptrdiff_t diff);

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
