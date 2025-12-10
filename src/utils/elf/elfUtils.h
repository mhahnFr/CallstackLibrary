/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef utils_elf_elfUtils_h
#define utils_elf_elfUtils_h

#include <endian.h>

/**
 * Converts the given number to the host endianness.
 *
 * @param bits the amount of bits of the given value
 * @param number the value in question
 * @param littleEndian whether the given number uses little endianness
 */
#define ELF_TO_HOST(bits, number, littleEndian) (littleEndian ? le##bits##toh(number) : be##bits##toh(number))

#endif /* utils_elf_elfUtils_h */
