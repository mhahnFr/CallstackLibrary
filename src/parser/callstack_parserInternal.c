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
#include "demangler.h"
#include "file/cache/cache.h"

#define _GNU_SOURCE
 #define __USE_GNU
  #include <dlfcn.h>
  #include <stdio.h>
 #undef __USE_GNU
#undef _GNU_SOURCE

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

bool callstack_parser_parseImpl(struct callstack_parser * self,
                                struct callstack *        callstack) {
    callstack->frames = malloc((callstack->backtraceSize + 1) * sizeof(struct callstack_frame *));
    if (callstack->frames == NULL) {
        return false;
    }
    callstack->frameCount = callstack->backtraceSize;
    for (size_t i = 0; i < callstack->backtraceSize; ++i) {
        struct callstack_frame * frame = callstack_frame_new();
        if (frame == NULL) {
            return false;
        }
        Dl_info info;
        if (dladdr(callstack->backtrace[i], &info)) {
            if ((frame->binaryFile = strdup(info.dli_fname)) == NULL) {
                callstack_frame_delete(frame);
                return false;
            }
            struct binaryFile * file = cache_findOrAddFile(callstack_parser_getCache(self), info.dli_fname);
            if (file == NULL || !file->addr2String(file,
                                                   &info,
                                                   callstack->backtrace[i],
                                                   frame)) {
                if (!callstack_parser_createDynamicLine(&info,
                                                        "<< Unknown >>",
                                                        callstack->backtrace[i],
                                                        frame)) {
                    callstack_frame_delete(frame);
                    return false;
                }
            }
        } else {
            if ((frame->function = strdup("<< Unknown >>")) == NULL) {
                callstack_frame_delete(frame);
                return false;
            }
        }
        callstack->frames[i] = frame;
    }
    return true;
}

bool callstack_parser_createDynamicLine(       Dl_info *         info,
                                               char *            fallback,
                                               void *            frameAddress,
                                        struct callstack_frame * frame) {
    if (info->dli_sname == NULL) {
        if ((frame->function = strdup(fallback)) == NULL) {
            return false;
        }
    } else {
        if ((frame->function = callstack_parser_createLine(info->dli_sname,
                                                           frameAddress - info->dli_saddr)) == NULL) {
            return false;
        }
    }
    return true;
}

char * callstack_parser_demangle(char * name) {
    char * result  = name;
    bool needsCopy = true;
    
#ifdef CXX_DEMANGLE
    result = callstack_demangle(result);
    if (result != name) {
        needsCopy = false;
    }
#endif
    
    return needsCopy ? strdup(result) : result;
}

char * callstack_parser_createLine(const char * name, ptrdiff_t diff) {
    char * result = (char *) name;
    bool del      = false;

#ifdef CXX_DEMANGLE
    result = callstack_demangle(result);
    if (result != name) {
        del = true;
    }
#endif

    char * ret;
    if (asprintf(&ret, "%s + %td", result, diff) < 0) {
        ret = strdup(result);
    }
    if (del) {
        free(result);
    }
    return ret;
}
