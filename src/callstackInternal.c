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

#include "callstackInternal.h"

#include <execinfo.h>
#include <string.h>

#include "dlMapper/dlMapper.h"
#include "parser/callstack_parser.h"

void callstack_createWithBacktrace(struct callstack * self,
                                   void * trace[], size_t traceLength) {
    *self = (struct callstack) CALLSTACK_INITIALIZER;
    traceLength = traceLength <= CALLSTACK_BACKTRACE_SIZE ? traceLength : CALLSTACK_BACKTRACE_SIZE;
    memcpy(self->backtrace, trace, traceLength * sizeof(void *));
    self->backtraceSize = traceLength;
}

int callstack_backtrace(void* buffer[], const int bufferSize, const void* address) {
    const int frames = backtrace(buffer, bufferSize);
    int i = 0;

    if (frames < 0) return frames;
    
#ifdef LCS_USE_BUILTINS
    for (i = 0; buffer[i] != address && i < bufferSize; ++i);
    (void) memmove(buffer, buffer + i, ((size_t) bufferSize - i) * sizeof(void *));
#else
    (void) address;
#endif
    return frames - i;
}

enum callstack_type callstack_translate(struct callstack * self) {
    if (self->frames == NULL && callstack_translateBinaries(self, false) == FAILED) {
        return FAILED;
    }
    
    struct callstack_parser parser;
    callstack_parser_create(&parser);
    self->translationStatus = callstack_parser_parse(&parser, self);
    callstack_parser_destroy(&parser);
    return self->translationStatus;
}

enum callstack_type callstack_translateBinaries(struct callstack* self, const bool useCache) {
    self->frames = malloc(sizeof(struct callstack_frame) * self->backtraceSize);
    if (self->frames == NULL) {
        return FAILED;
    }
    self->frameCount = self->backtraceSize;

    dlMapper_init();
    for (size_t i = 0; i < self->backtraceSize; ++i) {
        struct callstack_frame* element = &self->frames[i];

        callstack_frame_create(element);

        struct loadedLibInfo* info = dlMapper_libInfoForAddress(self->backtrace[i]);
        element->binaryFile = info == NULL ? NULL : useCache ? info->absoluteFileName : strdup(info->absoluteFileName);
        element->binaryFileRelative = info == NULL ? NULL : useCache ? info->relativeFileName : strdup(info->relativeFileName);
        element->binaryFileIsSelf = info == NULL ? false : info->isSelf;
        element->reserved = info;
        element->reserved1 = useCache;
    }
    return TRANSLATED;
}

void callstack_reset(struct callstack * self) {
    if (self->frames != NULL) {
        for (size_t i = 0; i < self->frameCount; ++i) {
            callstack_frame_destroy(&self->frames[i]);
        }
        free(self->frames);
        self->frameCount = 0;
    }
}
