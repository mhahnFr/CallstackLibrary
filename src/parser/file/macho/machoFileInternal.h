/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#ifndef machoFileInternal_h
#define machoFileInternal_h

#include "OptionalFuncFilePair.h"

#include "machoFile.h"

/**
 * Parses the Mach-O file represented by the given structure using the
 * given base address.
 *
 * @param self the Mach-O file structure representing the file to be parsed
 * @param baseAddress the base address of the Mach-O file to parse
 * @return whether the parsing was successful
 */
bool machoFile_parseFile(struct machoFile * self, void * baseAddress);

/**
 * Adds the givne object file structure to the given Mach-O file structure.
 *
 * @param self the Mach-O file structure
 * @param file the object file structure
 */
void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file);

/**
 * Searches and returns the function and the object file the function is in.
 *
 * @param self the Mach-O file instance
 * @param startAddress the start address of the Mach-O file
 * @param address the raw address whose function and object file to be found
 */
struct optional_funcFile machoFile_findFunction(struct machoFile * self,
                                                void *             startAddress,
                                                void *             address);

#endif /* machoFileInternal_h */
