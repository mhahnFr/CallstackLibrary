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

#ifndef leb128_h
#define leb128_h

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Reads an unsigned LEB128 integer from the given memory at the given position.
 *
 * The given memory position points to the first byte after the read number once this function returns.
 *
 * @param begin the memory pointer
 * @param counter the memory position
 * @return the deducted number
 */
uint64_t getULEB128(void* begin, size_t* counter);

/**
 * @brief Reads a signed LEB128 integer from the given memory at the given position.
 *
 * The given memory position points to the first byte after the read number once this function returns.
 *
 * @param begin the memory pointer
 * @param counter the memory position
 * @return the deducted number
 */
int64_t getLEB128(void* begin, size_t* counter);



#endif /* leb128_h */
