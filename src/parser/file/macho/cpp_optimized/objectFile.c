/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include <stdlib.h>

#include "../objectFile.h"
#include "../FunctionVector.h"

#include "../../dwarf/vector_dwarf_lineInfo.h"

/**
 * This structure acts as a wrapper around the object file structure.
 */
struct objectFile_private {
    /** The object file structure.                       */
    struct objectFile _;
    
    /** A vector with the functions of this object file. */
    struct vector_function functions;
    struct vector_function ownFunctions;
    struct vector_dwarfLineInfo lineInfos;
    const char* mainSourceFileCache;
};

struct objectFile * objectFile_new(void) {
    struct objectFile_private * self = malloc(sizeof(struct objectFile_private));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(&self->_);
    self->_.priv = self;
    vector_function_create(&self->functions);
    vector_function_create(&self->ownFunctions);
    vector_dwarfLineInfo_create(&self->lineInfos);
    self->mainSourceFileCache = NULL;
    return &self->_;
}

void objectFile_addFunction(struct objectFile * me,
                            struct function     function) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    vector_function_push_back(&self->functions, function);
}

void objectFile_addOwnFunction(struct objectFile* me,
                               struct function    function) {
    struct objectFile_private* self = me->priv;
    
    vector_function_push_back(&self->ownFunctions, function);
}

struct optional_function objectFile_findFunction(struct objectFile * me, uint64_t address) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    struct optional_function toReturn = { .has_value = false };
    
    size_t i;
    for (i = 0; i < self->functions.count && (address < self->functions.content[i].startAddress || address > self->functions.content[i].startAddress + self->functions.content[i].length); ++i);
    
    if (i < self->functions.count) {
        toReturn.has_value = true;
        toReturn.value     = self->functions.content[i];
    }
    
    return toReturn;
}

static inline void objectFile_dwarfLineCallback(struct dwarf_lineInfo info, va_list args) {
    struct objectFile_private* self = va_arg(args, void*);
    
    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

static inline bool objectFile_parseIntern(struct objectFile_private* self) {
    const bool result = objectFile_parse(&self->_, objectFile_dwarfLineCallback, self);
    if (!result) {
        for (size_t i = 0; i < self->ownFunctions.count; ++i) {
            function_destroy(&self->ownFunctions.content[i]);
        }
        vector_function_clear(&self->ownFunctions);
    }
    return result;
}

static inline optional_function_t objectFile_findOwnFunction(struct objectFile_private* self, const char* name) {
    optional_function_t toReturn = { .has_value = false };
    
    for (size_t i = 0; i < self->ownFunctions.count; ++i) {
        if (strcmp(name, self->ownFunctions.content[i].linkedName) == 0) {
            toReturn = (struct optional_function) { true, self->ownFunctions.content[i] };
            break;
        }
    }
    
    return toReturn;
}

static inline const char* objectFile_getSourceFileName(struct objectFile_private* self) {
    if (self->mainSourceFileCache != NULL) return self->mainSourceFileCache;
    
    const size_t size = strlen(self->_.directory) + strlen(self->_.name) + 1;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        // TODO: Handle this better
        return "";
    }
    strlcpy(toReturn, self->_.directory, size);
    strlcat(toReturn, self->_.sourceFile, size);
    return self->mainSourceFileCache = toReturn;
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* me, uint64_t address) {
    optional_debugInfo_t toReturn = { .has_value = false };
    optional_function_t func = objectFile_findFunction(me, address);
    if (!func.has_value) {
        return toReturn;
    }
    
    toReturn = (optional_debugInfo_t) {
        true, (struct debugInfo) {
            .functionName = func.value.linkedName,
            .sourceFileInfo.has_value = false
        }
    };
    struct objectFile_private* self = (struct objectFile_private*) me->priv;
    
    if (!me->parsed) {
        if (!(me->parsed = objectFile_parseIntern(self))) {
            return toReturn;
        }
    }
    optional_function_t ownFunction = objectFile_findOwnFunction(self, func.value.linkedName);
    if (!ownFunction.has_value) {
        return toReturn;
    }
    const uint64_t lineAddress = ownFunction.value.startAddress + address - func.value.startAddress;
    
    struct dwarf_lineInfo* closest = NULL;
    for (size_t i = 0; i < self->lineInfos.count; ++i) {
        struct dwarf_lineInfo* elem = &self->lineInfos.content[i];
        
        if (closest == NULL && elem->address < lineAddress) {
            closest = elem;
        } else if (closest != NULL && elem->address < lineAddress && lineAddress - elem->address < lineAddress - closest->address) {
            closest = elem;
        }
    }
    if (closest == NULL) {
        return toReturn;
    }
    toReturn.value.sourceFileInfo = (optional_sourceFileInfo_t) {
        true, (struct sourceFileInfo) {
            closest->line,
            closest->column,
            closest->fileName == NULL ? objectFile_getSourceFileName(self) : closest->fileName
        }
    };
    return toReturn;
}

void objectFile_functionsForEach(struct objectFile * me, void (*func)(struct function *, va_list), ...) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    va_list list;
    va_start(list, func);
    for (size_t i = 0; i < self->functions.count; ++i) {
        va_list copy;
        va_copy(copy, list);
        func(&self->functions.content[i], copy);
        va_end(copy);
    }
    va_end(list);
}

//static inline size_t getULEB128(void* begin, size_t* counter) {
//    uint64_t result = 0,
//             shift  = 0;
//    
//    bool more = true;
//    do {
//        uint8_t b = *((uint8_t *) (begin + *counter));
//        *counter += 1;
//        result |= (b & 0x7f) << shift;
//        shift += 7;
//        if (b < 0x80) {
//            more = false;
//        }
//    } while (more);
//    return result;
//}
//
//static inline int64_t getLEB128(void* begin, size_t* counter) {
//    int64_t result = 0,
//            shift  = 0;
//    
//    bool more = true;
//    do {
//        uint8_t b = *((uint8_t*) (begin + *counter));
//        *counter += 1;
//        result |= (b & 0x7f) << shift;
//        shift += 7;
//        if ((0x80 & b) == 0) {
//            if (shift < 32 && (b & 0x40) != 0) {
//                result |= ~0 << shift;
//            }
//            more = false;
//        }
//    } while (more);
//    return result;
//}
//
//static inline void processDwarfDebugLine(struct objectFile* self, void* begin) {
//    size_t counter = 0;
//    // First length: 32-bit uint
//    const uint32_t length = *((uint32_t*) begin);
//    __builtin_printf("Length: %u\n", length);
//    counter += 4;
//    __builtin_printf("Version number: %u\n", *((unsigned short*) (begin + counter)));
//    counter += 2;
//    // Branching here: Goto branch for the given DWARF version
//    // This example follows version 4 (32 bit). - mhahnFr
//    __builtin_printf("Header length: %u\n", *((unsigned int*) (begin + counter)));
//    counter += 4;
//    uint8_t min_instruction_length = *((uint8_t*) (begin + counter));
//    __builtin_printf("Instruction length (min): %u\n", min_instruction_length);
//    counter += 1;
//    uint8_t maximum_operations = *((uint8_t*) (begin + counter));
//    __builtin_printf("Instruction length (max): %u\n", maximum_operations);
//    counter += 1;
//    bool defaultStmt = *((unsigned char *) (begin + counter));
//    __builtin_printf("Is stmt (default): %s\n", defaultStmt ? "true" : "false");
//    counter += 1;
//    int8_t line_base = *((int8_t*) (begin + counter));
//    __builtin_printf("line base: %d\n", line_base);
//    counter += 1;
//    uint8_t line_range = *((uint8_t*) (begin + counter));
//    __builtin_printf("line range: %u\n", line_range);
//    counter += 1;
//    uint8_t opcodeBase = *((uint8_t *) (begin + counter));
//    __builtin_printf("opcode base: %u\n", opcodeBase);
//    counter += 1;
//    __builtin_printf("Opcode lengths:\n");
//    for (size_t i = 1; i < opcodeBase; ++i) {
//        __builtin_printf("    Opcode %zu: %u\n", i, *((uint8_t *) (begin + counter)));
//        counter += 1;
//    }
//    __builtin_printf("Directories:\n");
//    for (size_t i = 1; *((uint8_t *) (begin + counter)) != 0x0; ++i) {
//        const char* str = begin + counter;
//        __builtin_printf("    %zu: %s\n", i, str);
//        counter += strlen(str) + 1;
//    }
//    counter += 1;
//    __builtin_printf("Files:\n");
//    for (size_t i = 1; *((uint8_t *) (begin + counter)) != 0x0; ++i) {
//        const char* str = begin + counter;
//        counter += strlen(str) + 1;
//        
//        size_t dirIndex = getULEB128(begin, &counter);
//        size_t modification = getULEB128(begin, &counter);
//        size_t length = getULEB128(begin, &counter);
//        __builtin_printf("    %zu: %s, dirIndex: %zu, modification: %zu, length: %zu\n", i, str, dirIndex, modification, length);
//    }
//    counter += 1;
//    
//    // Parser part
//    uint64_t address = 0x0;
//    uint64_t op_index = 0;
//    uint64_t file = 1;
//    uint64_t line = 1;
//    uint64_t column = 0;
//    bool is_stmt = defaultStmt;
//    bool basic_block = false;
//    bool end_sequence = false;
//    bool prologue_end = false;
//    bool epilogue_begin = false;
//    uint64_t isa = 0;
//    uint64_t discriminator = 0;
//    while (counter - 4 < length) {
//        // Address            Line   Column File   ISA Discriminator Flags
//        
//        const uint8_t opcode = *((uint8_t*) (begin + counter));
//        counter += 1;
//        if (opcode == 0) {
//            // zero, uleb128 length, opcode, ...
//            const size_t length = getULEB128(begin, &counter);
//            const uint8_t opcode = *((uint8_t*) (begin + counter));
//            counter += 1;
//            switch (opcode) {
//                case 1:
//                    end_sequence = true;
//                    __builtin_printf("%llx %llu %llu %llu %llu %llu %s %s %s %s %s\n", address, line, column, file, isa, discriminator, is_stmt ? "is_stmt" : "", basic_block ? "basic_block" : "", end_sequence ? "end_sequence" : "", prologue_end ? "prologue_end" : "", epilogue_begin ? "epilogue_begin" : "");
//                    address = 0x0;
//                    op_index = 0;
//                    file = 1;
//                    line = 1;
//                    column = 0;
//                    is_stmt = defaultStmt;
//                    basic_block = false;
//                    end_sequence = false;
//                    prologue_end = false;
//                    epilogue_begin = false;
//                    isa = 0;
//                    discriminator = 0;
//                    break;
//                    
//                case 2: {
//                    size_t newAddress = *((size_t*) (begin + counter));
//                    counter += 8;
//                    address = newAddress;
//                    op_index = 0;
//                    break;
//                }
//                    
//                case 3: // TODO: Add another file
//                    abort();
//                    break;
//                    
//                case 4:
//                    discriminator = getULEB128(begin, &counter);
//                    break;
//                    
//                default: break; // Unknown op code, ignored
//            }
//        } else if (opcode < opcodeBase) {
//            switch (opcode) {
//                case  1:
//                    __builtin_printf("%llx %llu %llu %llu %llu %llu %s %s %s %s %s\n", address, line, column, file, isa, discriminator, is_stmt ? "is_stmt" : "", basic_block ? "basic_block" : "", end_sequence ? "end_sequence" : "", prologue_end ? "prologue_end" : "", epilogue_begin ? "epilogue_begin" : "");
//                    discriminator = 0;
//                    basic_block = prologue_end = epilogue_begin = false;
//                    break;
//                    
//                case  2: {
//                    const uint64_t value = getULEB128(begin, &counter);
//                    address += min_instruction_length * ((op_index + value) / maximum_operations);
//                    op_index = (op_index + value) % maximum_operations;
//                    break;
//                }
//                    
//                case  3:
//                    line += getLEB128(begin, &counter);
//                    break;
//                    
//                case  4:
//                    file = getULEB128(begin, &counter);
//                    break;
//                    
//                case  5:
//                    column = getULEB128(begin, &counter);
//                    break;
//                    
//                case  6:
//                    is_stmt = !is_stmt;
//                    break;
//                    
//                case  7:
//                    basic_block = true;
//                    break;
//                    
//                case  8: {
//                    uint8_t adjusted_opcode = 255 - opcodeBase;
//                    uint8_t op_adv = adjusted_opcode / line_range;
//                    
//                    address += min_instruction_length * ((op_index + op_adv) / maximum_operations);
//                    op_index = (op_index + op_adv) % maximum_operations;
//                    break;
//                }
//                    
//                case  9: {
//                    op_index = 0;
//                    uint16_t adder = *((uint16_t*) (begin + counter));
//                    counter += 2;
//                    address += adder;
//                    break;
//                }
//                    
//                case 10:
//                    prologue_end = true;
//                    break;
//                    
//                case 11:
//                    epilogue_begin = true;
//                    break;
//                    
//                case 12:
//                    isa = getULEB128(begin, &counter);
//                    break;
//                    
//                default: break; // Unknown op code, ignored
//            }
//        } else {
//            // Special opcode
//            uint8_t adjusted_opcode = opcode - opcodeBase;
//            uint8_t op_adv = adjusted_opcode / line_range;
//            
//            address += min_instruction_length * ((op_index + op_adv) / maximum_operations);
//            op_index = (op_index + op_adv) % maximum_operations;
//            line += line_base + (adjusted_opcode % line_range);
//            
//            __builtin_printf("%llx %llu %llu %llu %llu %llu %s %s %s %s %s\n", address, line, column, file, isa, discriminator, is_stmt ? "is_stmt" : "", basic_block ? "basic_block" : "", end_sequence ? "end_sequence" : "", prologue_end ? "prologue_end" : "", epilogue_begin ? "epilogue_begin" : "");
//            
//            basic_block = false;
//            prologue_end = false;
//            epilogue_begin = false;
//            discriminator = 0;
//        }
//    }
//    __builtin_printf("%zu %u\n", counter, length);
//}
//
//#include <assert.h>
//static inline bool objectFile_parse2(struct objectFile * self, void * buffer) {
//    struct mach_header_64 * header = buffer;
//    struct load_command *   lc     = (void*) header + sizeof(struct mach_header_64);
//    const  uint32_t         ncmds  = header->ncmds;
//    
//    assert(header->magic == MH_MAGIC_64);
//    
//    __builtin_printf("Sections in file: %s\n", self->name);
//    for (size_t i = 0; i < ncmds; ++i) {
//        switch (lc->cmd) {
//            case LC_SEGMENT_64: {
//                struct segment_command_64* command = (void*) lc;
//                
//                for (size_t i = 0; i < command->nsects; ++i) {
//                    struct section_64* curr = (void*) command + sizeof(struct segment_command_64) + i * sizeof(struct section_64);
//                    
//                    __builtin_printf("%s in %s, size: %llu\n", curr->sectname, curr->segname, curr->size);
//                    if (strcmp("__DWARF", curr->segname) == 0 &&
//                        strcmp("__debug_line", curr->sectname) == 0) {
//                        processDwarfDebugLine(self, buffer + curr->offset);
//                    }
//                }
//                __builtin_printf("---\n");
//                
//                break;
//            }
//        }
//        lc = (void*) lc + lc->cmdsize;
//    }
//    __builtin_printf("---\n");
//    
//    return true;
//}
//
//void objectFile_printParse(struct objectFile * self) {
//    if (self->name == NULL) return /*false*/;
//    
//    struct stat fileStats;
//    if (stat(self->name, &fileStats) != 0) {
//        return /*false*/;
//    }
//    void * buffer = malloc(fileStats.st_size);
//    if (buffer == NULL) {
//        return /*false*/;
//    }
//    FILE * file = fopen(self->name, "r");
//    const size_t count = fread(buffer, 1, fileStats.st_size, file);
//    fclose(file);
//    const bool success = (off_t) count == fileStats.st_size && objectFile_parse(self, buffer);
//    free(buffer);
//    return /*success*/;
//}

/**
 * Calls the destroy function for the given function object.
 *
 * @param f the function to be destroyed
 * @param args ignored
 */
static inline void objectFile_functionDestroy(struct function * f, va_list args) {
    (void) args;
    
    function_destroy(f);
}

void objectFile_destroy(struct objectFile * me) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;

    objectFile_functionsForEach(me, &objectFile_functionDestroy);
    vector_function_destroy(&self->functions);
    for (size_t i = 0; i < self->ownFunctions.count; ++i) {
        function_destroy(&self->ownFunctions.content[i]);
    }
    vector_function_destroy(&self->ownFunctions);
    for (size_t i = 0; i < self->lineInfos.count; ++i) {
        dwarf_lineInfo_destroy(&self->lineInfos.content[i]);
    }
    vector_dwarfLineInfo_destroy(&self->lineInfos);
    free((void*) self->mainSourceFileCache);
    free(me->sourceFile);
    free(me->directory);
    free(me->name);
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self->priv);
}
