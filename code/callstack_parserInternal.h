/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022  mhahnFr
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
 * @brief Attempts to parse the debug symbols in order to translate the given callstack.
 *
 * Returns FAILED if the debug symbols are unavailable. This function might use information
 * of the dynamic linker for some of the frames.
 *
 * @param self The callstack parser object.
 * @param callstack The callstack to be translated.
 * @return The status of the translation.
 */
enum callstack_type callstack_parser_parseDebugSymbols(struct callstack_parser * self,
                                                       struct callstack * callstack);

/**
 * @brief Parses the information of the dynamic linker in order to translate the given callstack.
 *
 * Returns FAILED if an error occurred.
 *
 * @param self The callstack parser object.
 * @param callstack The callstack object to be translated.
 * @return The status of the translation.
 */
enum callstack_type callstack_parser_parseDynamicLinker(struct callstack_parser * self,
                                                        struct callstack * callstack);

#endif /* callstack_parserInternal_h */
