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

#include <stdbool.h>
#include <string.h>

#include "demangler.h"

#include "swift/demangler.h"

#ifdef CXX_FUNCTIONS
# include "cxx/demangler.h"
#endif

bool activateSwiftDemangler = true;

/**
 * Returns whether the given name is a name mangled in the C++ style.
 *
 * @param name the name to be checked
 * @return whether the name is a mangled one
 */
static inline bool callstack_demangle_isCxx(const char* name) {
    if (strncmp(name, "_Z", 2) == 0 || strncmp(name, "___Z", 4) == 0) {
        return true;
    }
    return strlen(name) >= 11 && strncmp(name, "_GLOBAL_", 8) == 0
        && (name[8] == '.' || name[8] == '_' || name[8] == '$')
        && (name[9] == 'D' || name[9] == 'I')
        && name[10] == '_';
}

/**
 * Returns whether the given name is a name mangled in the Swift style.
 *
 * @param name the name to be checked
 * @return whether the name is a mangled one
 */
static inline bool callstack_demangle_isSwift(const char* name) {
    if (!activateSwiftDemangler) return false;

    return strncmp(name, "_$s", 3) == 0 || strncmp(name, "$s", 2) == 0
        || strncmp(name, "_$e", 3) == 0 || strncmp(name, "$e", 2) == 0
        || strncmp(name, "_$S", 3) == 0 || strncmp(name, "$S", 2) == 0
        || strncmp(name, "_T0", 3) == 0
        || strncmp(name, "@__swiftmacro_", 14) == 0;
}

char* callstack_demangle(char* name) {
    char* toReturn = name;
    if (callstack_demangle_isCxx(name)) {
#ifdef CXX_FUNCTIONS
        toReturn = callstack_demangle_cxx(name);
#endif
    } else if (callstack_demangle_isSwift(name)) {
        toReturn = callstack_demangle_swift(name);
    }
    return toReturn;
}
