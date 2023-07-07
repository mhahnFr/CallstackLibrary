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

#include "callstackInternal.h"

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

struct callstack * callstack_new(void) {
    return callstack_newWithAddress(__builtin_return_address(0));
}

struct callstack * callstack_newWithAddress(void * address) {
    void * trace[CALLSTACK_BACKTRACE_SIZE];
    int size = callstack_backtrace(trace, CALLSTACK_BACKTRACE_SIZE, address);
    if (size < 0) return NULL;
    
    struct callstack * ret = callstack_allocate();
    if (ret != NULL) {
        (void) callstack_createWithBacktrace(ret, trace, (size_t) size);
    }
    return ret;
}

bool callstack_emplace(struct callstack * self) {
    return callstack_emplaceWithAddress(self, __builtin_return_address(0));
}

bool callstack_emplaceWithAddress(struct callstack * self, void * address) {
    void * trace[CALLSTACK_BACKTRACE_SIZE];
    int size = callstack_backtrace(trace, CALLSTACK_BACKTRACE_SIZE, address);
    return callstack_emplaceWithBacktrace(self, trace, size);
}

bool callstack_emplaceWithBacktrace(struct callstack * self,
                                    void * trace[], int traceLength) {
    if (traceLength < 0) return false;
    
    callstack_createWithBacktrace(self, trace, (size_t) traceLength);
    return true;
}

void callstack_copy(struct callstack * self, const struct callstack * other) {
    if (other != self) {
        callstack_destroy(self);
        
        self->translationStatus  = other->translationStatus;
        self->backtraceSize      = other->backtraceSize;
        
        for (size_t i = 0; i < self->backtraceSize; ++i) {
            self->backtrace[i]   = other->backtrace[i];
        }
        
        self->stringArraySize    = other->stringArraySize;
        
        if (self->stringArraySize != 0) {
            self->stringArray    = malloc(self->stringArraySize * sizeof(char *));
        }
        
        for (size_t i = 0; i < self->stringArraySize; ++i) {
            self->stringArray[i] = strdup(other->stringArray[i]);
        }
    }
}

char ** callstack_toArray(struct callstack * self) {
    if (self == NULL) return NULL;

    if (self->translationStatus == NONE && callstack_translate(self) == FAILED) {
        return NULL;
    }
    return self->stringArray;
}

const char * callstack_toString(struct callstack * self, char separator) {
    if (self == NULL) return NULL;

    if (self->translationStatus == NONE && callstack_translate(self) == FAILED) {
        return NULL;
    } else if (self->stringArray == NULL || self->stringArraySize == 0) {
        char string[2] = { separator, '\0' };
        return strdup(string);
    }
    char * string = malloc(callstack_getTotalStringLength(self) + (self->stringArraySize + 1) * sizeof(char));
    size_t i, j;
    for (i = 0, j = 0; i < self->stringArraySize; ++i, ++j) {
        const size_t len = strlen(self->stringArray[i]);
        memcpy(string + j, self->stringArray[i], len);
        j += len;
        string[j] = separator;
    }
    string[j] = '\0';
    return string;
}

void callstack_destroy(struct callstack * self) {
    self->translationStatus = NONE;
    self->backtraceSize     = 0;
    
    if (self->stringArray != NULL) {
        for (size_t i = 0; i < self->stringArraySize; ++i) {
            free(self->stringArray[i]);
        }
        free(self->stringArray);
    }
    
    self->stringArraySize = 0;
}

void callstack_delete(struct callstack * self) {
    callstack_destroy(self);
    free(self);
}
