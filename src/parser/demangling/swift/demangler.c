/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2025  mhahnFr
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

#include <functionInfo/functionInfo.h>

#include "demangler.h"

typedef char* (*SwiftDemanglerFunc)(const char*, size_t, char*, size_t*, uint32_t);

static inline SwiftDemanglerFunc callstack_demangle_getSwiftDemangler(void) {
    static struct {
        SwiftDemanglerFunc func;
        bool searched;
    } loadingState = { NULL, false };

    if (!loadingState.searched) {
        loadingState.searched = true;
        struct functionInfo info = functionInfo_load("swift_demangle");
        if (info.found) {
            loadingState.func = (void*) info.begin;
        }
    }
    return loadingState.func;
}

char* callstack_demangle_swift(char* name) {
    char* toReturn = name;

    SwiftDemanglerFunc swift_demangle = callstack_demangle_getSwiftDemangler();
    if (swift_demangle != NULL) {
        char* demangled = swift_demangle(name, strlen(name), NULL, NULL, 0);
        if (demangled != NULL) {
            toReturn = demangled;
        }
    }

    return toReturn;
}
