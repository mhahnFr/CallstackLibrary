/*
 * Callstack Library - Library creating human-readable call stacks.
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

#include "callstack_parserInternal.h"

#include "../callstackInternal.h"
#include "file/cache/cache.h"

enum callstack_type callstack_parser_parse(struct callstack_parser * self,
                                           struct callstack *        callstack) {
    if (!callstack_parser_parseImpl(self, callstack)) {
        callstack_reset(callstack);
        return FAILED;
    }
    
    return TRANSLATED;
}

void callstack_parser_destroy(struct callstack_parser * self) {
    cache_clear(&self->parsedFiles);
}
