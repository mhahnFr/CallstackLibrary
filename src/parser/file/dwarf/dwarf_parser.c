/*
 * Callstack Library - Library creating human-readable call stacks.
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dwarf_parser.h"
#include "v5/parser.h"

#include "fileNameEntry.h"
#include "vector_string.h"
#include "vector_uint8.h"

uint64_t getULEB128(void* begin, size_t* counter) {
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

int64_t getLEB128(void* begin, size_t* counter) {
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
                result |= ((uint64_t) ~0 << shift);
            }
            more = false;
        }
    } while (more);
    return result;
}

/**
 * @brief Constructs an allocated string from the given file name entry using the given directories.
 *
 * @param file the file name entry
 * @param directories the vector with the directories
 * @return an allocated full path or `NULL` if the allocation failed or the file entry's name was `NULL`
 */
static inline char* dwarf_stringFrom(struct dwarf_fileNameEntry* file, struct vector_string* directories) {
    if (file->dirIndex == 0) {
        return file->name == NULL ? NULL : strdup(file->name);
    }
    const char* directory = directories->content[file->dirIndex - 1];
    const size_t size  = strlen(directory) + strlen(file->name) + 2;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return NULL;
    }
    strncpy(toReturn, directory, size);
    strncat(toReturn, "/", size);
    strncat(toReturn, file->name, size);
    toReturn[size - 1] = '\0';
    return toReturn;
}

/**
 * Parses the DWARF line program as specified in version 4.
 *
 * @param counter the start of the line program in the given memory
 * @param actualSize the total size of the line program
 * @param bit64 whether to parse as 64 Bit version
 * @param cb the callback to be called when a line entry has been deducted
 * @param args the arguments to pass to the callback
 */
static inline bool dwarf4_parseLineProgram(struct lcs_section debugLine,
                                           struct lcs_section debugLineStr,
                                           struct lcs_section debugStr,
                                           size_t   counter,
                                           uint64_t actualSize,
                                           bool     bit64,
                                           dwarf_line_callback cb, va_list args) {
    uint64_t headerLength;
    if (bit64) {
        headerLength = *((uint64_t*) (debugLine.content + counter));
        counter += 8;
    } else {
        headerLength = *((uint32_t*) (debugLine.content + counter));
        counter += 4;
    }
    (void) headerLength;
    
    const uint8_t minimumInstructionLength = *((uint8_t*) (debugLine.content + counter++));
    const uint8_t maximumOperations        = *((uint8_t*) (debugLine.content + counter++));
    const bool    defaultIsStmt            = *((uint8_t*) (debugLine.content + counter++));
    const int8_t  lineBase                 = *((int8_t*)  (debugLine.content + counter++));
    const uint8_t lineRange                = *((uint8_t*) (debugLine.content + counter++));
    const uint8_t opCodeBase               = *((uint8_t*) (debugLine.content + counter++));

    vector_uint8_t stdOpcodeLengths;
    vector_uint8_create(&stdOpcodeLengths);
    vector_uint8_reserve(&stdOpcodeLengths, opCodeBase - 2);
    for (uint8_t i = 1; i < opCodeBase; ++i) {
        vector_uint8_push_back(&stdOpcodeLengths, *((uint8_t*) (debugLine.content + counter++)));
    }
    
    vector_string_t includeDirectories;
    vector_string_create(&includeDirectories);
    while (*((uint8_t*) (debugLine.content + counter)) != 0x0) {
        const char* string = debugLine.content + counter;
        vector_string_push_back(&includeDirectories, string);
        counter += strlen(string) + 1;
    }
    ++counter;
    
    vector_dwarfFileEntry_t fileNames;
    vector_dwarfFileEntry_create(&fileNames);
    while (*((uint8_t*) (debugLine.content + counter)) != 0x0) {
        const char* string = debugLine.content + counter;
        counter += strlen(string) + 1;
        
        const uint64_t dirIndex     = getULEB128(debugLine.content, &counter),
                       modification = getULEB128(debugLine.content, &counter),
                       size         = getULEB128(debugLine.content, &counter);
        vector_dwarfFileEntry_push_back(&fileNames, (struct dwarf_fileNameEntry) { string, dirIndex, modification, size });
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
        const uint8_t opCode = *((uint8_t*) (debugLine.content + counter++));
        if (opCode == 0) {
            const uint64_t length = getULEB128(debugLine.content, &counter);
            const uint8_t  actualOpCode = *((uint8_t*) (debugLine.content + counter++));
            switch (actualOpCode) {
                case 1: {
                    endSequence = true;
                    va_list copy;
                    va_copy(copy, args);
                    cb((struct dwarf_lineInfo) {
                        address, line, column, isa, discriminator,
                        file == 0 ? NULL : dwarf_stringFrom(&fileNames.content[file - 1], &includeDirectories),
                        isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
                    }, copy);
                    va_end(copy);
                    
                    address = opIndex = column = isa = discriminator = 0;
                    basicBlock = endSequence = prologueEnd = epilogueBegin = false;
                    file = line = 1;
                    isStmt = defaultIsStmt;
                    break;
                }
                    
                case 2: {
                    const size_t newAddress = *((size_t*) (debugLine.content + counter));
                    counter += sizeof(size_t);
                    address = newAddress;
                    opIndex = 0;
                    break;
                }
                    
//                case 3: // TODO: Add another file
//                    break;
                    
                case 4: discriminator = getULEB128(debugLine.content, &counter); break;

                default: counter += length - 1; break;
            }
        } else if (opCode < opCodeBase) {
            switch (opCode) {
                case 1: {
                    va_list copy;
                    va_copy(copy, args);
                    cb((struct dwarf_lineInfo) {
                        address, line, column, isa, discriminator,
                        file == 0 ? NULL : dwarf_stringFrom(&fileNames.content[file - 1], &includeDirectories),
                        isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
                    }, copy);
                    va_end(copy);
                    
                    discriminator = 0;
                    basicBlock = prologueEnd = epilogueBegin = false;
                    break;
                }
                    
                case 2: {
                    const uint64_t operationAdvance = getULEB128(debugLine.content, &counter);
                    address += minimumInstructionLength * ((opIndex + operationAdvance) / maximumOperations);
                    opIndex = (opIndex + operationAdvance) % maximumOperations;
                    break;
                }
                    
                case 3: line += getLEB128(debugLine.content, &counter);   break;
                case 4: file = getULEB128(debugLine.content, &counter);   break;
                case 5: column = getULEB128(debugLine.content, &counter); break;
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
                    const uint16_t adder = *((uint16_t*) (debugLine.content + counter));
                    counter += 2;
                    address += adder;
                    break;
                }
                    
                case 10: prologueEnd = true;                break;
                case 11: epilogueBegin = true;              break;
                case 12: isa = getULEB128(debugLine.content, &counter); break;

                default:
                    for (uint64_t i = 0; i < stdOpcodeLengths.content[opCode - 1]; ++i) {
                        getLEB128(debugLine.content, &counter);
                    }
                    break;
            }
        } else {
            uint8_t adjustedOpCode   = opCode - opCodeBase;
            uint8_t operationAdvance = adjustedOpCode / lineRange;
            
            address += minimumInstructionLength * ((opIndex + operationAdvance) / maximumOperations);
            opIndex  = (opIndex + operationAdvance) % maximumOperations;
            line    += lineBase + (adjustedOpCode % lineRange);
            
            va_list copy;
            va_copy(copy, args);
            cb((struct dwarf_lineInfo) {
                address, line, column, isa, discriminator,
                file == 0 ? NULL : dwarf_stringFrom(&fileNames.content[file - 1], &includeDirectories),
                isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
            }, copy);
            va_end(copy);
            
            basicBlock    = false;
            prologueEnd   = false;
            epilogueBegin = false;
            discriminator = 0;
        }
    }
    
    vector_uint8_destroy(&stdOpcodeLengths);
    vector_string_destroy(&includeDirectories);
    vector_dwarfFileEntry_destroy(&fileNames);
    
    if (counter < debugLine.size - 2 - (bit64 ? 12 : 4)) {
        return dwarf_parseLineProgram((struct lcs_section) {
            debugLine.content + counter,
            debugLine.size - 2 - (bit64 ? 12 : 4) - counter
        }, debugLineStr, debugStr, cb, args);
    }
    return true;
}

bool dwarf_parseLineProgram(struct lcs_section debugLine,
                            struct lcs_section debugLineStr,
                            struct lcs_section debugStr,
                            dwarf_line_callback cb, va_list args) {
    size_t counter = 0;
    
    const uint32_t size = *((uint32_t*) debugLine.content);
    counter += 4;
    
    bool     bit64;
    uint64_t actualSize;
    if (size == 0xffffffff) {
        actualSize = *((uint64_t*) (debugLine.content + counter));
        bit64      = true;
        counter += 8;
    } else {
        actualSize = size;
        bit64      = false;
    }
    const uint16_t version = *((uint16_t*) (debugLine.content + counter));
    counter += 2;
    
    switch (version) {
        case 4: return dwarf4_parseLineProgram(debugLine, debugLineStr, debugStr, counter, actualSize, bit64, cb, args);
        case 5: return dwarf5_parseLineProgram(debugLine, debugLineStr, debugStr, counter, actualSize, bit64, cb, args);

        default: return false;
    }
    return false;
}
