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

#include "callstack_parserInternal.h"

enum callstack_type callstack_parser_parse(struct callstack_parser * self,
                                           struct callstack * callstack) {
    self->mode = DEBUG_SYMBOLS;
    enum callstack_type result = callstack_parser_parseDebugSymbols(self, callstack);
    if (result == FAILED) {
        self->mode = DYNAMIC_LINKER;
        result = callstack_parser_parseDynamicLinker(self, callstack);
    }
    return result;
}
