/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dlfcn.h>

#include <stdlib.h>

#include "callstackInternal.h"
#include "lcs_builtins.h"
#include "dlMapper/dlMapper.h"
#include "parser/file/vector_string.h"

struct callstack* callstack_new(void) {
    return callstack_newWithAddress(lcs_returnAddress(0));
}

struct callstack* callstack_newWithAddress(const void* address) {
    void * trace[CALLSTACK_BACKTRACE_SIZE];
    const int size = callstack_backtrace(trace, CALLSTACK_BACKTRACE_SIZE, address);
    if (size < 0) return NULL;
    
    struct callstack * ret = callstack_allocate();
    if (ret != NULL) {
        callstack_createWithBacktrace(ret, trace, (size_t) size);
    }
    return ret;
}

bool callstack_emplace(struct callstack* self) {
    return callstack_emplaceWithAddress(self, lcs_returnAddress(0));
}

bool callstack_emplaceWithAddress(struct callstack* self, const void* address) {
    void * trace[CALLSTACK_BACKTRACE_SIZE];
    const int size = callstack_backtrace(trace, CALLSTACK_BACKTRACE_SIZE, address);
    return callstack_emplaceWithBacktrace(self, trace, size);
}

bool callstack_emplaceWithBacktrace(struct callstack * self,
                                    void* trace[], const int traceLength) {
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
        
        self->frameCount = other->frameCount;
        if (self->frameCount != 0) {
            self->frames = malloc(self->frameCount * sizeof(struct callstack_frame));
            for (size_t i = 0; i < self->frameCount; ++i) {
                callstack_frame_copyHere(&self->frames[i], &other->frames[i]);
            }
        }
    }
}

bool callstack_relativize(struct callstack* self, const char** binaryNames) {
    if (self == NULL) return false;

    dlMapper_init();
    for (size_t i = 0; i < self->backtraceSize; ++i) {
        const pair_relativeInfo_t info = dlMapper_relativize(self->backtrace[i]);
        if (info.first == NULL) {
            return false;
        }
        self->backtrace[i] = (void*) info.second;
        binaryNames[i] = info.first->absoluteFileName;
    }
    return true;
}

struct callstack_frame* callstack_translateRelative(struct callstack* self, const char** binaryNames) {
    if (self == NULL) return NULL;

    vector_string_t handles = vector_initializer;
    vector_reserve(&handles, self->backtraceSize);
    for (size_t i = 0; i < self->backtraceSize; ++i) {
        vector_push_back(&handles, dlopen(binaryNames[i], RTLD_NOW));
    }
    dlMapper_init();
    for (size_t i = 0; i < self->backtraceSize; ++i) {
        self->backtrace[i] = dlMapper_absolutize(self->backtrace[i], binaryNames[i]);
    }
    self->frames = callstack_toArray(self);
    vector_iterate(&handles, if (*element != NULL) {
        dlclose((void*) *element);
    });
    return self->frames;
}

struct callstack_frame * callstack_toArray(struct callstack * self) {
    if (self == NULL) return NULL;

    if (self->translationStatus == NONE && callstack_translate(self) == FAILED) {
        return NULL;
    }
    return self->frames;
}

static inline struct callstack_frame* callstack_getBinariesShared(struct callstack* self, const bool useCache) {
    if (self == NULL) return NULL;

    if ((self->translationStatus == NONE || self->translationStatus == FAILED)
        && callstack_translateBinaries(self, useCache) == FAILED) {
        return NULL;
    }
    return self->frames;
}

struct callstack_frame* callstack_getBinaries(struct callstack* self) {
    return callstack_getBinariesShared(self, false);
}

struct callstack_frame* callstack_getBinariesCached(struct callstack* self) {
    return callstack_getBinariesShared(self, true);
}

void callstack_destroy(struct callstack * self) {
    self->translationStatus = NONE;
    self->backtraceSize     = 0;
    
    if (self->frames != NULL) {
        for (size_t i = 0; i < self->frameCount; ++i) {
            callstack_frame_destroy(&self->frames[i]);
        }
        free(self->frames);
    }
    
    self->frameCount = 0;
}

void callstack_delete(struct callstack * self) {
    callstack_destroy(self);
    free(self);
}
