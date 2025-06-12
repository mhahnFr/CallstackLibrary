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

#include "parser.h"
#include "definitions.h"
#include "optional_vector_fileAttribute.h"

#include "../dwarf_parser.h"
#include "../leb128.h"
#include "../vector_pair_uint64.h"

#include "../../optional_uint64_t.h"

/**
 * Reads an index that follows in the given data buffer.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the requested data type
 * @return the read index or an empty optional if the requested data type was not allowed
 */
static inline optional_uint64_t dwarf5_readIndex(void* buffer, size_t* counter, const uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_data1:
            toReturn = *(uint8_t*) (buffer + (*counter)++);
            break;

        case DW_FORM_data2:
            toReturn = *(uint16_t*) (buffer + *counter);
            *counter += 2;
            break;

        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        default: return (optional_uint64_t) { .has_value = false };
    }
    return (optional_uint64_t) { true, toReturn };
}

/**
 * Reads a timestamp information that follows in the data buffer.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the requested data type
 * @return the read timestamp or an empty optional if the given data type was not allowed
 */
static inline optional_uint64_t dwarf5_readTimestamp(void* buffer, size_t* counter, const uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data4:
            toReturn = *(uint32_t*) (buffer + *counter);
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *(uint64_t*) (buffer + *counter);
            *counter += 8;
            break;

        case DW_FORM_block: {
            const uint64_t length = getULEB128(buffer, counter);
            *counter += length;
            toReturn = 0; // Proprietary timestamp format not supported -> skipped.  - mhahnFr
            break;
        }

        default: return (optional_uint64_t) { .has_value = false };
    }
    return (optional_uint64_t) { true, toReturn };
}

/**
 * Reads a size information that follows in the data buffer.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the requested data type
 * @return the read size value or an empty optional if the given data type was not allowed
 */
static inline optional_uint64_t dwarf5_readSize(void* buffer, size_t* counter, const uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data1:
            toReturn = *(uint8_t*) (buffer + (*counter)++);
            break;

        case DW_FORM_data2:
            toReturn = *(uint16_t*) (buffer + *counter);
            *counter += 2;
            break;

        case DW_FORM_data4:
            toReturn = *(uint32_t*) (buffer + *counter);
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *(uint64_t*) (buffer + *counter);
            *counter += 8;
            break;

        default: return (optional_uint64_t) { .has_value = false };
    }
    return (optional_uint64_t) { true, toReturn };
}

/**
 * Reads the MD5 hash that follows in the buffer.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @return the read MD5 hash
 */
static inline uint8_t* dwarf5_readMD5(void* buffer, size_t* counter) {
    uint8_t* toReturn = buffer + *counter;
    *counter += 16;
    return toReturn;
}

/**
 * Parses the following file attributes in the line program header.
 *
 * @param self the generified parser object
 * @param counter the reading index
 * @return the parsed file attributes or an empty optional if the parsing failed
 */
static inline optional_vector_fileAttribute_t dwarf5_parseFileAttributes(struct dwarf_parser* self, size_t* counter) {
    const uint8_t entryFormatCount = *(uint8_t*) (self->debugLine.content + (*counter)++);
    vector_pair_uint64_t entryFormats = vector_initializer;
    vector_reserve(&entryFormats, entryFormatCount);
    for (uint8_t i = 0; i < entryFormatCount; ++i) {
        const uint64_t contentType = getULEB128(self->debugLine.content, counter),
                          formCode = getULEB128(self->debugLine.content, counter);
        vector_push_back(&entryFormats, ((pair_uint64_t) { contentType, formCode }));
    }

    const uint64_t attributeCount = getULEB128(self->debugLine.content, counter);
    vector_fileAttribute_t attributes = vector_initializer;
    vector_reserve(&attributes, attributeCount);
    for (uint64_t i = 0; i < attributeCount; ++i) {
        struct fileAttribute attribute = {
            .path = NULL,
            .md5  = NULL,
            .size      = 0,
            .index     = 0,
            .timestamp = 0,
        };
        vector_iterate(&entryFormats, {
            switch (element->first) {
                case DW_LNCT_path:
                    attribute.path = dwarf_readString(self, self->debugLine.content, counter, element->second);
                    break;

                case DW_LNCT_directory_index: {
                    const optional_uint64_t value = dwarf5_readIndex(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.index = value.value;
                    break;
                }

                case DW_LNCT_timestamp: {
                    const optional_uint64_t value = dwarf5_readTimestamp(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.timestamp = value.value;
                    break;
                }

                case DW_LNCT_size: {
                    const optional_uint64_t value = dwarf5_readSize(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.size = value.value;
                    break;
                }

                case DW_LNCT_MD5:
                    if (element->second != DW_FORM_data16) goto fail;
                    attribute.md5 = dwarf5_readMD5(self->debugLine.content, counter);
                    break;

                default:
                    if (!dwarf_consumeSome(self, self->debugLine.content, counter, element->second)) goto fail;
                    break;
            }
        });
        vector_push_back(&attributes, attribute);
    }
    vector_destroy(&entryFormats);

    return (optional_vector_fileAttribute_t) { true, attributes };

fail:
    vector_destroy(&entryFormats);
    vector_destroy(&attributes);

    return (optional_vector_fileAttribute_t) { .has_value = false };
}

/**
 * Constructs the full file name for the given source file attribute using the given include directory attributes.
 *
 * @param file the source file attribute to construct the full file name for
 * @param directories the include directory file attributes
 * @param defaultDirectory the compilation directory
 * @return the allocated full source file path or `NULL` if the allocation failed
 */
static inline char* dwarf5_constructFileName(const struct fileAttribute*   file,
                                             const vector_fileAttribute_t* directories,
                                             const char*                   defaultDirectory) {
    bool freeDir = false;
    const char* dirPath = directories->content[file->index].path;
    if (*dirPath != '/') {
        dirPath = dwarf_pathConcatenate(defaultDirectory, dirPath);
        if (dirPath == NULL) {
            return NULL;
        }
        freeDir = true;
    }
    char* toReturn = dwarf_pathConcatenate(dirPath, file->path);
    if (freeDir) {
        free((char*) dirPath);
    }
    return toReturn;
}

/**
 * Creates a source file reference for the given source file index.
 *
 * @param self the generified parser object
 * @param file the file index
 * @return the source file reference
 */
static inline struct dwarf_sourceFile dwarf5_getFileName(const struct dwarf_parser* self, const uint64_t file) {
    const struct fileAttribute* filePtr = &self->specific.v5.files.content[file];
    return (struct dwarf_sourceFile) {
        dwarf5_constructFileName(filePtr, &self->specific.v5.directories, self->compilationDirectory),
        NULL, NULL,
        filePtr->timestamp,
        filePtr->size
    };
}

/**
 * Parses the DWARF line program header in version 5.
 *
 * @param self the generified parser object
 * @param counter the reading index
 * @return whether the parsing was successful
 */
static inline bool dwarf5_parseLineProgramHeader(struct dwarf_parser* self, size_t* counter) {
    const uint8_t addressSize = *(uint8_t*) (self->debugLine.content + (*counter)++);
    const uint8_t segmentSelectorSize = *(uint8_t*) (self->debugLine.content + (*counter)++);
    (void) addressSize;
    (void) segmentSelectorSize;

    uint64_t headerLength;
    if (self->bit64) {
        headerLength = *(uint64_t*) (self->debugLine.content + *counter);
        *counter += 8;
    } else {
        headerLength = *(uint32_t*) (self->debugLine.content + *counter);
        *counter += 4;
    }
    (void) headerLength;

    self->minimumInstructionLength        = *(uint8_t*) (self->debugLine.content + (*counter)++);
    self->maximumOperationsPerInstruction = *(uint8_t*) (self->debugLine.content + (*counter)++);
    self->defaultIsStmt                   = *(uint8_t*) (self->debugLine.content + (*counter)++);
    self->lineBase                        = *(int8_t*)  (self->debugLine.content + (*counter)++);
    self->lineRange                       = *(uint8_t*) (self->debugLine.content + (*counter)++);
    self->opCodeBase                      = *(uint8_t*) (self->debugLine.content + (*counter)++);

    vector_reserve(&self->stdOpcodeLengths, self->opCodeBase - 1);
    for (uint8_t i = 1; i < self->opCodeBase; ++i) {
        vector_push_back(&self->stdOpcodeLengths, *((uint8_t*) (self->debugLine.content + (*counter)++)));
    }

    const optional_vector_fileAttribute_t maybeDirs = dwarf5_parseFileAttributes(self, counter);
    if (!maybeDirs.has_value) {
        return false;
    }
    self->specific.v5.directories = maybeDirs.value;

    const optional_vector_fileAttribute_t maybeFiles = dwarf5_parseFileAttributes(self, counter);
    if (!maybeFiles.has_value) {
        return false;
    }
    self->specific.v5.files = maybeFiles.value;
    return true;
}

/**
 * Destroys the DWARF 5 specific part of the given generified parser object.
 *
 * @param self the generified parser object
 */
static inline void dwarf5_parser_destroy(const struct dwarf_parser* self) {
    vector_destroy(&self->specific.v5.files);
    vector_destroy(&self->specific.v5.directories);
}

void dwarf5_parser_create(struct dwarf_parser* self) {
    self->specific.v5 = (struct dwarf5_parser) {
        .directories = vector_initializer,
        .files       = vector_initializer
    };
    self->destroy     = dwarf5_parser_destroy;
    self->parseHeader = dwarf5_parseLineProgramHeader;
    self->getFileName = dwarf5_getFileName;
}
