/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#ifndef macho_utils_h
#define macho_utils_h

/**
 * Changes the endianness of the given value if requested.
 *
 * @param bits the amount of bits of the given value
 * @param swap whether to swap the bytes
 * @param value the value in question
 * @return the usable value
 */
#define macho_maybeSwap(bits, swap, value) ((swap) ? OSSwapInt##bits(value) : (value))

/**
 * @brief Iterates over the segments in the given Mach-O runtime image and executes
 * the given block of code for each of them.
 *
 * The segment is available with the variable named @c loadCommand .
 *
 * @param baseAddr the base address of the runtime image
 * @param bytesSwapped whether to swap the endianness
 * @param suffix the suffix for the system structure names to be used
 * @param block the block to be executed for each segment
 */
#define macho_iterateSegments(baseAddr, bytesSwapped, suffix, block) {                               \
    const struct mach_header##suffix* _header = (void*) (baseAddr);                                  \
    struct load_command* loadCommand = (void*) _header + sizeof(*_header);                           \
    const uint32_t _cmdCount = macho_maybeSwap(32, bytesSwapped, _header->ncmds);                    \
    for (uint32_t _i = 0; _i < _cmdCount; ++_i) {                                                    \
        { block }                                                                                    \
        loadCommand = (void*) loadCommand + macho_maybeSwap(32, bytesSwapped, loadCommand->cmdsize); \
    }                                                                                                \
}

/**
 * @brief Iterates over the sections in the given Mach-O segment and executes
 * the given block of code for each of them.
 *
 * The section is available using the variable named @c section .
 *
 * @param segment the segment whose sections to iterate
 * @param bytesSwapped whether to swap the endianness
 * @param suffix the suffix for the system structure names to be used
 * @param block the block to be executed for each section
 */
#define macho_iterateSections(segment, bytesSwapped, suffix, block) {                   \
    const struct segment_command##suffix* _segment = (void*) (segment);                 \
    const uint32_t _sectionCount = macho_maybeSwap(32, bytesSwapped, _segment->nsects); \
    for (uint32_t _j = 0; _j < _sectionCount; ++_j) {                                   \
        const struct section##suffix* section = ((void*) _segment) + sizeof(*_segment)  \
                                                + _j * sizeof(struct section##suffix);  \
        { block }                                                                       \
    }                                                                                   \
}

#endif /* macho_utils_h */