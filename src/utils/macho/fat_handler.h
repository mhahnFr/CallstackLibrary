/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef fat_handler_h
#define fat_handler_h

#include <stdbool.h>

#include <mach-o/fat.h>

/**
 * Extracts the appropriate Mach-O slice in the given fat archive.
 *
 * @param fatHeader the header of the fat archive
 * @param bitsReversed whether the bytes need to be reversed to match the host byte order
 * @param fileName the name of the represented Mach-O file
 * @return the slice the system would load or `NULL` if no appropriate slice is found
 */
void* macho_parseFat(const struct fat_header* fatHeader, bool bitsReversed, const char* fileName);

#endif /* fat_handler_h */
