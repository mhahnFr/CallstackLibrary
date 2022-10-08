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
#include "defs.h"

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

struct callstack * callstack_new() {
    void * trace[CALLSTACK_BACKTRACE_SIZE];
    size_t size = backtrace(trace, CALLSTACK_BACKTRACE_SIZE);
    
    struct callstack * ret = callstack_allocate(sizeof(struct callstack) + size * sizeof(void *));
    if (ret != NULL) {
        callstack_createWithBacktrace(ret, trace, size);
    }
    return ret;
}

struct callstack * callstack_allocate(size_t size) {
    return malloc(size);
}

void callstack_create(struct callstack * self) {
    self->backtraceSize     = 0;
    self->stringArraySize   = 0;
    self->stringArray       = NULL;
    self->translationStatus = NONE;
}

void callstack_createWithBacktrace(struct callstack * self,
                                   void * trace[], size_t traceLength) {
    callstack_create(self);
    memcpy(self->backtrace, trace, traceLength);
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
