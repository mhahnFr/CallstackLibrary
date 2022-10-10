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
#include "../include/callstack_defs.h"

#include <string.h>

void callstack_createWithBacktrace(struct callstack * self,
                                   void * trace[], size_t traceLength) {
    callstack_create(self);
    traceLength = traceLength <= CALLSTACK_BACKTRACE_SIZE ? traceLength : CALLSTACK_BACKTRACE_SIZE;
    memcpy(self->backtrace, trace, traceLength * sizeof(void *));
    self->backtraceSize = traceLength;
}

void callstack_destroy(struct callstack * self) {
    self->translationStatus = NONE;
    self->backtraceSize     = 0;
    
    if (self->stringArray != NULL) {
        for (size_t i = 0; i < self->stringArraySize; ++i) {
            free(self->stringArray[i]);
        }
    }
    
    self->stringArraySize = 0;
}

enum callstack_type callstack_translate(struct callstack * self) {
    // TODO: Implement
    self->translationStatus = FAILED;
    return FAILED;
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
