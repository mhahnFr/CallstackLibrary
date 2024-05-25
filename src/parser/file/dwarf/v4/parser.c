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

#include "parser.h"

#include "../dwarf_parser.h"

static inline bool dwarf4_parseLineProgramHeader(struct dwarf_parser* self, size_t* counter) {
    uint64_t headerLength;
    if (self->bit64) {
        headerLength = *((uint64_t*) (self->debugLine.content + *counter));
        *counter += 8;
    } else {
        headerLength = *((uint32_t*) (self->debugLine.content + *counter));
        *counter += 4;
    }
    (void) headerLength;

    self->minimumInstructionLength        = *((uint8_t*) (self->debugLine.content + (*counter)++));
    if (self->version == 4) {
        self->maximumOperationsPerInstruction = *((uint8_t*) (self->debugLine.content + (*counter)++));
    }
    self->defaultIsStmt                   = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->lineBase                        = *((int8_t*)  (self->debugLine.content + (*counter)++));
    self->lineRange                       = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->opCodeBase                      = *((uint8_t*) (self->debugLine.content + (*counter)++));

    vector_uint8_reserve(&self->stdOpcodeLengths, self->opCodeBase - 2);
    for (uint8_t i = 1; i < self->opCodeBase; ++i) {
        vector_uint8_push_back(&self->stdOpcodeLengths, *((uint8_t*) (self->debugLine.content + (*counter)++)));
    }

    while (*((uint8_t*) (self->debugLine.content + *counter)) != 0x0) {
        const char* string = self->debugLine.content + *counter;
        vector_string_push_back(&self->specific.v4.includeDirectories, string);
        *counter += strlen(string) + 1;
    }
    ++(*counter);

    while (*((uint8_t*) (self->debugLine.content + *counter)) != 0x0) {
        const char* string = self->debugLine.content + *counter;
        *counter += strlen(string) + 1;

        const uint64_t dirIndex     = getULEB128(self->debugLine.content, counter),
                       modification = getULEB128(self->debugLine.content, counter),
                       size         = getULEB128(self->debugLine.content, counter);
        vector_dwarfFileEntry_push_back(&self->specific.v4.fileNames, (struct dwarf_fileNameEntry) {
            string, dirIndex, modification, size
        });
    }
    ++(*counter);
    return true;
}

static inline char* dwarf4_stringFrom(struct dwarf_fileNameEntry* file, struct vector_string* directories) {
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

static inline struct dwarf_sourceFile dwarf4_parser_getFileName(struct dwarf_parser* self, uint64_t file) {
    struct dwarf_fileNameEntry* filePtr = file == 0 ? NULL : &self->specific.v4.fileNames.content[file - 1];
    return (struct dwarf_sourceFile) {
        filePtr == NULL ? NULL : dwarf4_stringFrom(filePtr, &self->specific.v4.includeDirectories),
        filePtr->modTime,
        filePtr->size
    };
}

static inline void dwarf4_parser_destroy(struct dwarf_parser* self) {
    vector_string_destroy(&self->specific.v4.includeDirectories);
    vector_dwarfFileEntry_destroy(&self->specific.v4.fileNames);
}

void dwarf4_parser_create(struct dwarf_parser* self) {
    self->specific.v4 = (struct dwarf4_parser) {
        .fileNames          = vector_initializer,
        .includeDirectories = vector_initializer
    };
    self->destroy     = dwarf4_parser_destroy;
    self->parseHeader = dwarf4_parseLineProgramHeader;
    self->getFileName = dwarf4_parser_getFileName;
}
