/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024, 2026  mhahnFr
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

#ifndef archive_h
#define archive_h

#include "objectFile.h"

/**
 * @brief The function signature of the callback function called when an
 * object file was extracted.
 *
 * Takes an object file as parameter.
 */
typedef void (*macho_archive_callback)(struct objectFile*);

/**
 * @brief Parses the given archive file.
 *
 * Calls the callback once an object file has been extracted and parsed.
 *
 * @param fileName the name of the archive file
 * @param cb the callback to be called with an object file
 */
void macho_archive_parse(const char* fileName, macho_archive_callback cb);

#endif /* archive_h */
