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

#include "callstack_parserInternal.h"
#include "demangler.h"
#include "file/cache/cache.h"

#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum callstack_type callstack_parser_parseDebugSymbols(struct callstack_parser * self,
                                                       struct callstack * callstack) {
    enum callstack_type ret = self->mode;
    callstack->stringArray = malloc((callstack->backtraceSize + 1) * sizeof(char *));
    if (callstack->stringArray == NULL) {
        return FAILED;
    }
    callstack->stringArraySize = callstack->backtraceSize;
    for (size_t i = 0; i < callstack->backtraceSize; ++i) {
        Dl_info info;
        if (dladdr(callstack->backtrace[i], &info)) {
            struct binaryFile * file = cache_findOrAddFile(info.dli_fname);
            if (file == NULL || (callstack->stringArray[i] = file->addr2String(file, &info)) == NULL) {
                ret = FAILED;
                break;
            }
        } else {
            if ((callstack->stringArray[i] = strdup("<Unknown>")) == NULL) {
                ret = FAILED;
                break;
            }
        }
    }
    return ret;
}

enum callstack_type callstack_parser_parseDynamicLinker(struct callstack_parser * self,
                                                        struct callstack * callstack) {
    enum callstack_type ret = self->mode;
    callstack->stringArray = malloc((callstack->backtraceSize + 1) * sizeof(char *));
    if (callstack->stringArray == NULL) {
        return FAILED;
    }
    callstack->stringArraySize = callstack->backtraceSize;
    char ** strings = backtrace_symbols(callstack->backtrace, (int) callstack->backtraceSize);
    if (strings == NULL) {
        return FAILED;
    }
    size_t i;
    for (i = 0; i < callstack->backtraceSize; ++i) {
        Dl_info info;
        if (dladdr(callstack->backtrace[i], &info)) {
            if (info.dli_sname == NULL) {
                if ((callstack->stringArray[i] = strdup(strings[i])) == NULL) {
                    ret = FAILED;
                    break;
                }
            } else {
                if ((callstack->stringArray[i] = callstack_parser_demangle(info.dli_sname,
                                                                           callstack->backtrace[i] - info.dli_saddr)) == NULL) {
                    ret = FAILED;
                    break;
                }
            }
        } else {
            if ((callstack->stringArray[i] = strdup("<Unknown>")) == NULL) {
                ret = FAILED;
                break;
            }
        }
    }
    callstack->stringArray[i] = NULL;
    free(strings);
    return ret;
}

char * callstack_parser_demangle(const char * name, ptrdiff_t diff) {
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
