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

#include "callstackInternal.h"
#include "parser/callstack_parser.h"
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
    (void) memmove(buffer, buffer + i, ((size_t) bufferSize - i) * sizeof(void *));
    return frames - i;
}

enum callstack_type callstack_translate(struct callstack * self) {
    if (self->frames == NULL && callstack_translateBinaries(self) == FAILED) {
        return FAILED;
    }
    
    struct callstack_parser parser;
    callstack_parser_create(&parser);
    self->translationStatus = callstack_parser_parse(&parser, self);
    callstack_parser_destroy(&parser);
    return self->translationStatus;
}

enum callstack_type callstack_translateBinaries(struct callstack * self) {
    self->frames = malloc(sizeof(struct callstack_frame) * self->backtraceSize);
    if (self->frames == NULL) {
        return FAILED;
    }
    self->frameCount = self->backtraceSize;
    
    for (size_t i = 0; i < self->backtraceSize; ++i) {
        callstack_frame_create(&self->frames[i]);
        const bool success = self->frames[i].info.has_value
                           = dladdr(self->backtrace[i], &self->frames[i].info.value);
        const char * absolutePath = self->frames[i].info.value.dli_fname;
        self->frames[i].binaryFile = success ? binaryFile_toAbsolutePath((char *) absolutePath) : NULL;
        self->frames[i].binaryFileRelative = success ? binaryFile_toRelativePath((char *) absolutePath) : NULL;
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
