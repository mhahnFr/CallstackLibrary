/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef debugInfo_h
#define debugInfo_h

#include <stdint.h>

#include "../../../DC4C/optional.h"

struct sourceFileInfo {
    uint64_t line;
    uint64_t column;
    
    const char* sourceFile;
};

typedef_optional_named(sourceFileInfo, struct sourceFileInfo);

struct debugInfo {
    const char* functionName;
    
    optional_sourceFileInfo_t sourceFileInfo;
};

typedef_optional_named(debugInfo, struct debugInfo);

#endif /* debugInfo_h */
