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

#include "callstackInternal.h"
#include "callstack_parser.h"
#include "../include/callstack_defs.h"

#include <execinfo.h>
#include <string.h>

void callstack_createWithBacktrace(struct callstack * self,
                                   void * trace[], size_t traceLength) {
    callstack_create(self);
    traceLength = traceLength <= CALLSTACK_BACKTRACE_SIZE ? traceLength : CALLSTACK_BACKTRACE_SIZE;
    memcpy(self->backtrace, trace, traceLength * sizeof(void *));
    self->backtraceSize = traceLength;
}

int callstack_backtrace(void * buffer[], int bufferSize, void * address) {
    int i,
        frames = backtrace(buffer, bufferSize);
    
    if (frames < 0) return frames;
    
    for (i = 0; buffer[i] != address && i < bufferSize; ++i);
    (void) memmove(buffer, buffer + i, (size_t) bufferSize - i);
    return frames - i;
}

enum callstack_type callstack_translate(struct callstack * self) {
    struct callstack_parser parser;
    callstack_parser_create(&parser);
    self->translationStatus = callstack_parser_parse(&parser, self);
    callstack_parser_destroy(&parser);
    return self->translationStatus;
}

size_t callstack_getTotalStringLength(struct callstack * self) {
    size_t ret = 0;
    if (callstack_isTranslated(self)) {
        for (size_t i = 0; i < self->stringArraySize; ++i) {
            ret += strlen(self->stringArray[i]);
        }
    }
    return ret;
}
