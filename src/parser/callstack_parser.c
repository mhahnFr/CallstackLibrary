/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

#include "callstack_parser.h"
#include "demangler.h"
#include "lcs_stdio.h"

#include "../callstackInternal.h"

/**
 * @brief Creates a callstack line, demangling the given name if possible and enabled.
 *
 * An allocated callstack line is returned.
 *
 * @param name the name to be tried to demangled
 * @param diff the function depth
 * @return the callstack line or `NULL` on error
 */
static inline char* callstack_parser_createLine(const char* name, ptrdiff_t diff) {
    char* result = (char*) name;
    bool del     = false;

#ifdef CXX_FUNCTIONS
    result = callstack_demangle(result);
    if (result != name) {
        del = true;
    }
#endif

    char* ret;
    if (asprintf(&ret, "%s + %td", result, diff) < 0) {
        ret = strdup(result);
    }
    if (del) {
        free(result);
    }
    return ret;
}

/**
 * @brief Creates a callstack line using the given info and frame address.
 *
 * The given frame address is translated using the given dynamic linker information.
 *
 * @param info the dynamic linker info
 * @param frameAddress the translated frame's address
 * @param frame the frame structure to store the information in
 * @return whether the line could be set
 */
static inline bool callstack_parser_createDynamicLine(Dl_info*         info,
                                                      void*            frameAddress,
                                               struct callstack_frame* frame) {
    if (info->dli_sname != NULL &&
        (frame->function = callstack_parser_createLine(info->dli_sname,
                                                       frameAddress - info->dli_saddr)) == NULL) {
        return false;
    }
    return true;
}

/**
 * @brief Translates the given callstack using the given parser.
 *
 * @param self the callstack parser to be used
 * @param callstack the callstack to be translated
 * @return whether the translation was successful
 */
static inline bool callstack_parser_parseImpl(struct callstack_parser* self,
                                              struct callstack*        callstack) {
    (void) self;
    
    for (size_t i = 0; i < callstack->backtraceSize; ++i) {
        optional_Dl_info_t*     info  = &callstack->frames[i].info;
        struct callstack_frame* frame = &callstack->frames[i];
        
        if (!info->has_value) continue;
        
        struct binaryFile* file = binaryFile_findOrAddFile(info->value.dli_fname,
                                                           info->value.dli_fbase);
        if (file == NULL || !file->addr2String(file, callstack->backtrace[i], frame)) {
            if (!callstack_parser_createDynamicLine(&info->value,
                                                    callstack->backtrace[i],
                                                    frame)) {
                return false;
            }
        }
    }
    return true;
}

char* callstack_parser_demangle(char* name) {
    char* result   = name;
    bool needsCopy = true;
    
#ifdef CXX_FUNCTIONS
    result = callstack_demangle(result);
    if (result != name) {
        needsCopy = false;
    }
#endif
    
    return needsCopy ? strdup(result) : result;
}

enum callstack_type callstack_parser_parse(struct callstack_parser * self,
                                           struct callstack *        callstack) {
    if (!callstack_parser_parseImpl(self, callstack)) {
        callstack_reset(callstack);
        return FAILED;
    }
    
    return TRANSLATED;
}
