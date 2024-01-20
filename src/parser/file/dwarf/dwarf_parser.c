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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dwarf_parser.h"

#include "../../../../DC4C/vector.h"

struct dwarfFileNameEntry {
    const char* name;
    uint64_t dirIndex;
    uint64_t modTime;
    uint64_t size;
};

typedef_vector_light_named(uint8, uint8_t);
typedef_vector_light_named(string, const char*);
typedef_vector_light_named(dwarfFile, struct dwarfFileNameEntry);

static inline uint64_t getULEB128(void* begin, size_t* counter) {
    uint64_t result = 0,
             shift  = 0;
    
    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if (b < 0x80) {
            more = false;
        }
    } while (more);
    return result;
}

static inline int64_t getLEB128(void* begin, size_t* counter) {
    int64_t result = 0,
            shift  = 0;
    
    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if ((0x80 & b) == 0) {
            if (shift < 32 && (b & 0x40) != 0) {
                result |= ~0 << shift;
            }
            more = false;
        }
    } while (more);
    return result;
}

static inline bool dwarf_parseLineProgramV4(void* begin, size_t counter, uint64_t actualSize, bool bit64, dwarf_line_callback cb) {
    uint64_t headerLength;
    if (bit64) {
        headerLength = *((uint64_t*) (begin + counter));
        counter += 8;
    } else {
        headerLength = *((uint32_t*) (begin + counter));
        counter += 4;
    }
    const uint8_t minimumInstructionLength = *((uint8_t*) (begin + counter++));
    const uint8_t maximumOperations        = *((uint8_t*) (begin + counter++));
    const bool    defaultIsStmt            = *((uint8_t*) (begin + counter++));
    const int8_t  lineBase                 = *((int8_t*)  (begin + counter++));
    const uint8_t lineRange                = *((uint8_t*) (begin + counter++));
    const uint8_t opCodeBase               = *((uint8_t*) (begin + counter++));
    
    // TODO: For DC4C: Create copy create function
    vector_uint8_t stdOpcodeLengths;
    vector_uint8_create(&stdOpcodeLengths);
    vector_uint8_reserve(&stdOpcodeLengths, opCodeBase - 2);
    for (uint8_t i = 1; i < opCodeBase; ++i) {
        vector_uint8_push_back(&stdOpcodeLengths, *((uint8_t*) (begin + counter++)));
    }
    
    vector_string_t includeDirectories; // Treat 0 as 1!
    vector_string_create(&includeDirectories);
    while (*((uint8_t*) (begin + counter)) != 0x0) {
        const char* string = begin + counter;
        vector_string_push_back(&includeDirectories, string);
        counter += strlen(string) + 1;
    }
    ++counter;
    
    vector_dwarfFile_t fileNames; // Treat 0 as 1!
    vector_dwarfFile_create(&fileNames);
    while (*((uint8_t*) (begin + counter)) != 0x0) {
        const char* string = begin + counter;
        counter += strlen(string) + 1;
        
        const uint64_t dirIndex     = getULEB128(begin, &counter),
                       modification = getULEB128(begin, &counter),
                       size         = getULEB128(begin, &counter);
        vector_dwarfFile_push_back(&fileNames, (struct dwarfFileNameEntry) { string, dirIndex, modification, size });
    }
    ++counter;
    
    uint64_t address       = 0,
             opIndex       = 0,
             file          = 1,
             line          = 1,
             column        = 0,
             isa           = 0,
             discriminator = 0;
    
    bool isStmt         = defaultIsStmt,
          basicBlock    = false,
          endSequence   = false,
          prologueEnd   = false,
          epilogueBegin = false;
    
    while (counter - 4 < actualSize) {
        const uint8_t opCode = *((uint8_t*) (begin + counter++));
        if (opCode == 0) {
            const uint64_t length = getULEB128(begin, &counter);
            const uint8_t  actualOpCode = *((uint8_t*) (begin + counter++));
            switch (actualOpCode) {
                case 1:
                    endSequence = true;
                    // TODO: Call the callback with a new matrix line
                    address = opIndex = column = isa = discriminator = 0;
                    basicBlock = endSequence = prologueEnd = epilogueBegin = false;
                    file = line = 1;
                    isStmt = defaultIsStmt;
                    break;
                    
                case 2: {
                    const size_t newAddress = *((size_t*) (begin + counter));
                    counter += sizeof(size_t);
                    address = newAddress;
                    opIndex = 0;
                    break;
                }
                    
                case 3: // TODO: Add another file
                    abort();
                    break;
                    
                case 4: discriminator = getULEB128(begin, &counter); break;
                    
                default: counter += length - 1; break;
            }
        } else if (opCode < opCodeBase) {
            switch (opCode) {
                case 1:
                    // TODO: Call the callback with a new matrix line
                    discriminator = 0;
                    basicBlock = prologueEnd = epilogueBegin = false;
                    break;
                    
                case 2: {
                    const uint64_t operationAdvance = getULEB128(begin, &counter);
                    address += minimumInstructionLength * ((opIndex + operationAdvance) / maximumOperations);
                    opIndex = (opIndex + operationAdvance) % maximumOperations;
                    break;
                }
                    
                case 3: line += getLEB128(begin, &counter);   break;
                case 4: file = getULEB128(begin, &counter);   break;
                case 5: column = getULEB128(begin, &counter); break;
                case 6: isStmt = !isStmt;                     break;
                case 7: basicBlock = true;                    break;
                    
                case 8: {
                    const uint8_t adjustedOpCode   = 255 - opCodeBase;
                    const uint8_t operationAdvance = adjustedOpCode / lineRange;
                    
                    address += minimumInstructionLength * ((opIndex + operationAdvance) / maximumOperations);
                    opIndex  = (opIndex + operationAdvance) % maximumOperations;
                    break;
                }
                    
                case 9: {
                    opIndex = 0;
                    const uint16_t adder = *((uint16_t*) (begin + counter));
                    counter += 2;
                    address += adder;
                    break;
                }
                    
                case 10: prologueEnd = true;                break;
                case 11: epilogueBegin = true;              break;
                case 12: isa = getULEB128(begin, &counter); break;
                    
                default:
                    for (uint64_t i = 0; i < stdOpcodeLengths.content[opCode - 1]; ++i) {
                        getLEB128(begin, &counter);
                    }
                    break;
            }
        } else {
            uint8_t adjustedOpCode   = opCode - opCodeBase;
            uint8_t operationAdvance = adjustedOpCode / lineRange;
            
            address += minimumInstructionLength * ((opIndex + operationAdvance) / maximumOperations);
            opIndex  = (opIndex + operationAdvance) % maximumOperations;
            line    += lineBase + (adjustedOpCode % lineRange);
            
            // TODO: Call the callback with a new matrix line
            
            basicBlock    = false;
            prologueEnd   = false;
            epilogueBegin = false;
            discriminator = 0;
        }
    }
    
    return true;
}

bool dwarf_parseLineProgram(void* begin, dwarf_line_callback cb) {
    size_t counter = 0;
    
    const uint32_t size = *((uint32_t*) begin);
    counter += 4;
    
    bool     bit64;
    uint64_t actualSize;
    if (size == 0xffffffff) {
        actualSize = *((uint64_t*) (begin + counter));
        bit64      = true;
        counter += 8;
    } else {
        actualSize = size;
        bit64      = false;
    }
    const uint16_t version = *((uint16_t*) (begin + counter));
    counter += 2;
    
    switch (version) {
        case 4: return dwarf_parseLineProgramV4(begin, counter, actualSize, bit64, cb);
            
        default: return false;
    }
    return false;
}
