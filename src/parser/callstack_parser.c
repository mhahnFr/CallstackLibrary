/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2026  mhahnFr
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

#include "callstack_parser.h"

#include "../callstackInternal.h"
#include "demangling/demangler.h"
#include "file/binaryFile.h"

/**
 * @brief Translates the given callstack using the given parser.
 *
 * @param self the callstack parser to be used
 * @param callstack the callstack to be translated
 * @return whether the translation was successful
 */
static inline bool callstack_parser_parseImpl(const struct callstack_parser* self,
                                              const struct callstack*        callstack) {
    (void) self;
    
    for (size_t i = 0; i < callstack->backtraceSize; ++i) {
        struct binaryFile* file = callstack->frames[i].reserved;
        if (file == NULL) {
            continue;
        }
        if (!binaryFile_addr2String(file, callstack->backtrace[i], &callstack->frames[i])) {
            return false;
        }
    }
    return true;
}

char* callstack_parser_demangleCopy(char* name, const bool copy) {
    char* result   = name;
    bool needsCopy = true;

    result = callstack_demangle(result);
    if (result != name) {
        needsCopy = false;
    }

    return needsCopy ? (copy ? strdup(result) : NULL) : result;
}

enum callstack_type callstack_parser_parse(const struct callstack_parser* self,
                                           struct callstack* callstack) {
    if (!callstack_parser_parseImpl(self, callstack)) {
        callstack_reset(callstack);
        return FAILED;
    }
    
    return TRANSLATED;
}
