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

#include <string.h>

#include "callstack_parser.h"
#include "demangler.h"

#include "../callstackInternal.h"
#include "../loadedLibInfo.h"

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
        struct loadedLibInfo*    info = callstack->frames[i].reserved;
        struct callstack_frame* frame = &callstack->frames[i];

        if (info == NULL) continue;

        if (info->associated == NULL) {
            info->associated = binaryFile_new(info->fileName, info->begin);
        }
        struct binaryFile* file = info->associated;
        if (file == NULL) continue;

        file->relocationOffset = info->relocationOffset;
        file->inMemory = true;

        binaryFile_addr2String(file, callstack->backtrace[i], frame);
    }
    return true;
}

#ifdef CXX_FUNCTIONS
/**
 * Returns whether the given name is a name mangled in the C++ style.
 *
 * @param name the name to be checked
 * @return whether the name is a mangled one
 */
static inline bool callstack_parser_isMangledName(const char* name) {
    if (strncmp(name, "_Z", 2) == 0 || strncmp(name, "___Z", 4) == 0) {
        return true;
    }
    return strlen(name) >= 11 && strncmp(name, "_GLOBAL_", 8) == 0
        && (name[8] == '.' || name[8] == '_' || name[8] == '$')
        && (name[9] == 'D' || name[9] == 'I')
        && name[10] == '_';
}
#endif

char* callstack_parser_demangleCopy(char* name, bool copy) {
    char* result   = name;
    bool needsCopy = true;
    
#ifdef CXX_FUNCTIONS
    if (callstack_parser_isMangledName(name)) {
        result = callstack_demangle(result);
        if (result != name) {
            needsCopy = false;
        }
    }
#endif
    
    return needsCopy ? (copy ? strdup(result) : NULL) : result;
}

enum callstack_type callstack_parser_parse(struct callstack_parser * self,
                                           struct callstack *        callstack) {
    if (!callstack_parser_parseImpl(self, callstack)) {
        callstack_reset(callstack);
        return FAILED;
    }
    
    return TRANSLATED;
}
