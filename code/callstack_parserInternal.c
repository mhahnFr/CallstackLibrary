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

#include "callstack_parserInternal.h"
#include "demangler.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

enum callstack_type callstack_parser_parseDebugSymbols(struct callstack_parser * self,
                                                       struct callstack * callstack) {
    (void) self;
    (void) callstack;
    return FAILED;
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
                if ((callstack->stringArray[i] = callstack_parser_demangle(info.dli_sname)) == NULL) {
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

char * callstack_parser_demangle(const char * name) {
#ifdef CXX_DEMANGLE
    char * ret = callstack_demangle((char *) name);
    if (ret != name) {
        return ret;
    }
#endif
    return strdup(name);
}
