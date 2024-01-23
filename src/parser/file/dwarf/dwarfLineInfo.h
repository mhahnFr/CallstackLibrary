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

#ifndef dwarfLineInfo_h
#define dwarfLineInfo_h

#include <stdbool.h>
#include <stdint.h>

struct dwarfLineInfo {
    uint64_t address;
    uint64_t line;
    uint64_t column;
    uint64_t isa;
    uint64_t discriminator;
    
    const char* fileName;
    
    bool isStmt;
    bool basicBlock;
    bool endSequence;
    bool prologueEnd;
    bool epilogueBegin;
};

#endif /* dwarfLineInfo_h */
