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

#include "definitions.h"
#include "optional_uint64_t.h"
#include "optional_vector_fileAttribute.h"
#include "parser.h"
#include "vector_pair_uint64.h"

#include "../dwarf_parser.h"

/**
 * Calculates a string pointer into one of the given sections.
 *
 * @param offset the string offset
 * @param type the type of string to load
 * @param debugLineStr the section corresponding to the .debug_line_str section
 * @param debugStr the section corresponding to the .debug_str section
 * @return a pointer to the string in either section or `NULL` if the given type specifies neither section
 */
static inline char* dwarf5_stringFromSection(uint64_t offset,
                                             uint64_t type,
                                             struct lcs_section debugLineStr,
                                             struct lcs_section debugStr) {
    char* toReturn = NULL;
    switch (type) {
        case DW_FORM_line_strp: toReturn = debugLineStr.content + offset; break;
        case DW_FORM_strp:      toReturn = debugStr.content + offset;     break;
    }
    return toReturn;
}

/**
 * @brief Reads a string.
 *
 * The string may follow in the given data buffer or may come from one of the debug string sections.
 * The returned string is not allocated.
 *
 * @param buffer the data buffer
 * @param counter the reading index into the given data buffer
 * @param type the type of string to load
 * @param bit64 whether the 64 bit DWARF format is used
 * @param debugLineStr the section corresponding to the .debug_line_str section
 * @param debugStr the section corresponding to the .debug_str section
 * @return a pointer to the string which points into either the given data buffer or into one of the given sections;
 * `NULL` is returned if the given data type was not allowed
 */
static inline char* dwarf5_readString(void*    buffer,
                                      size_t*  counter,
                                      uint64_t type,
                                      bool     bit64,
                                      struct lcs_section debugLineStr,
                                      struct lcs_section debugStr) {
    if (type == DW_FORM_string) {
        char* toReturn = (buffer + *counter);
        *counter += strlen(toReturn) + 1;
        return toReturn;
    }
    if (type != DW_FORM_line_strp && type != DW_FORM_strp && type != DW_FORM_strp_sup) {
        return NULL;
    }
    uint64_t offset;
    if (bit64) {
        offset = *((uint64_t*) (buffer + *counter));
        *counter += 8;
    } else {
        offset = *((uint32_t*) (buffer + *counter));
        *counter += 4;
    }
    return dwarf5_stringFromSection(offset, type, debugLineStr, debugStr);
}

/**
 * Reads an index that follows in the given data buffer.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the requested data type
 * @return the read index or an empty optional if the requested data type was not allowed
 */
static inline optional_uint64_t dwarf5_readIndex(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_data1:
            toReturn = *((uint8_t*) (buffer + (*counter)++));
            break;

        case DW_FORM_data2:
            toReturn = *((uint16_t*) (buffer + *counter));
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
static inline optional_uint64_t dwarf5_readTimestamp(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data4:
            toReturn = *((uint32_t*) (buffer + *counter));
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *((uint64_t*) (buffer + *counter));
            *counter += 8;
            break;

        case DW_FORM_block: {
            const uint64_t length = getULEB128(buffer, counter);
            *counter += length;
            toReturn = 0; // Propriatary timestamp format not supported -> skipped.  - mhahnFr
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
static inline optional_uint64_t dwarf5_readSize(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data1:
            toReturn = *((uint8_t*) (buffer + (*counter)++));
            break;

        case DW_FORM_data2:
            toReturn = *((uint16_t*) (buffer + *counter));
            *counter += 2;
            break;

        case DW_FORM_data4:
            toReturn = *((uint32_t*) (buffer + *counter));
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *((uint64_t*) (buffer + *counter));
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
 * @param counter the reding index
 * @return the read MD5 hash
 */
static inline uint8_t* dwarf5_readMD5(void* buffer, size_t* counter) {
    uint8_t* toReturn = buffer + *counter;
    *counter += 16;
    return toReturn;
}

/**
 * Consumes the following data block of different possible types, according to the
 * formats available for additional vendor specific data.
 *
 * @param buffer the data buffer
 * @param counter the reading index
 * @param type the expected data type
 * @param bit64 whether to use the 64 bit format
 * @return whether the data was allowed and skipped successfully
 */
static inline bool dwarf5_consumeSome(void* buffer, size_t* counter, uint64_t type, bool bit64) {
    switch (type) {
        case DW_FORM_block: {
            const uint64_t length = getULEB128(buffer, counter);
            *counter += length;
            break;
        }

        case DW_FORM_block1: {
            uint8_t length = *((uint8_t*) (buffer + (*counter)++));
            *counter += length;
            break;
        }

        case DW_FORM_block2: {
            uint16_t length = *((uint16_t*) (buffer + *counter));
            *counter += 2 + length;
            break;
        }

        case DW_FORM_block4: {
            uint32_t length = *((uint32_t*) (buffer + *counter));
            *counter += 4 + length;
            break;
        }

        case DW_FORM_flag:
        case DW_FORM_strx1:
        case DW_FORM_data1:  ++(*counter);   break;
        case DW_FORM_strx2:
        case DW_FORM_data2:  *counter += 2;  break;
        case DW_FORM_strx3:  *counter += 3;  break;
        case DW_FORM_strx4:
        case DW_FORM_data4:  *counter += 4;  break;
        case DW_FORM_data8:  *counter += 8;  break;
        case DW_FORM_data16: *counter += 16; break;

        case DW_FORM_strp:
        case DW_FORM_string:
        case DW_FORM_line_strp:
            (void) dwarf5_readString(buffer, counter, type, bit64,
                                     (struct lcs_section) { .content = NULL, .size = 0 },
                                     (struct lcs_section) { .content = NULL, .size = 0 });
            break;

        case DW_FORM_sdata: getLEB128(buffer, counter);  break;

        case DW_FORM_strx:
        case DW_FORM_udata: getULEB128(buffer, counter); break;

        case DW_FORM_sec_offset: *counter += bit64 ? 8 : 4; break;

        default: return false;
    }
    return true;
}

/**
 * Parses the following file attributes in the line program header.
 *
 * @param self the generified parser object
 * @param counter the reading index
 * @return the parsed file attributes or an empty optional if the parsing failed
 */
static inline optional_vector_fileAttribute_t dwarf5_parseFileAttributes(struct dwarf_parser* self, size_t* counter) {
    const uint8_t entryFormatCount = *((uint8_t*) (self->debugLine.content + (*counter)++));
    vector_pair_uint64_t entryFormats = vector_initializer;
    vector_pair_uint64_reserve(&entryFormats, entryFormatCount);
    for (uint8_t i = 0; i < entryFormatCount; ++i) {
        const uint64_t contentType = getULEB128(self->debugLine.content, counter),
                          formCode = getULEB128(self->debugLine.content, counter);
        vector_pair_uint64_push_back(&entryFormats, (pair_uint64_t) { contentType, formCode });
    }

    const uint64_t attributeCount = getULEB128(self->debugLine.content, counter);
    vector_fileAttribute_t attributes = vector_initializer;
    vector_fileAttribute_reserve(&attributes, attributeCount);
    for (uint64_t i = 0; i < attributeCount; ++i) {
        struct fileAttribute attribute = {
            .path = NULL,
            .md5  = NULL,
            .size      = 0,
            .index     = 0,
            .timestamp = 0,
        };
        vector_iterate(pair_uint64_t, &entryFormats, {
            switch (element->first) {
                case DW_LNCT_path:
                    attribute.path = dwarf5_readString(self->debugLine.content, counter, element->second, self->bit64, self->debugLineStr, self->debugStr);
                    break;

                case DW_LNCT_directory_index: {
                    optional_uint64_t value = dwarf5_readIndex(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.index = value.value;
                    break;
                }

                case DW_LNCT_timestamp: {
                    optional_uint64_t value = dwarf5_readTimestamp(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.timestamp = value.value;
                    break;
                }

                case DW_LNCT_size: {
                    optional_uint64_t value = dwarf5_readSize(self->debugLine.content, counter, element->second);
                    if (!value.has_value) goto fail;
                    attribute.size = value.value;
                    break;
                }

                case DW_LNCT_MD5:
                    if (element->second != DW_FORM_data16) goto fail;
                    attribute.md5 = dwarf5_readMD5(self->debugLine.content, counter);
                    break;

                default:
                    if (!dwarf5_consumeSome(self->debugLine.content, counter, element->second, self->bit64)) goto fail;
                    break;
            }
        })
        vector_fileAttribute_push_back(&attributes, attribute);
    }
    vector_pair_uint64_destroy(&entryFormats);

    return (optional_vector_fileAttribute_t) { true, attributes };

fail:
    vector_pair_uint64_destroy(&entryFormats);
    vector_fileAttribute_destroy(&attributes);

    return (optional_vector_fileAttribute_t) { .has_value = false };
}

/**
 * Constructs the full file name for the given source file attribute using the given include directory attributes.
 *
 * @param file the source file attribute to construct the full file name for
 * @param directories the inlcude directory file attributes
 * @return the allocated full source file path or `NULL` if the allocation failed
 */
static inline char* dwarf5_constructFileName(const struct fileAttribute* file, const vector_fileAttribute_t* directories) {
    const char* dirPath = directories->content[file->index].path;
    const size_t size = strlen(dirPath) + strlen(file->path) + 2;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return NULL;
    }
    strncpy(toReturn, dirPath, size);
    strncat(toReturn, "/", size);
    strncat(toReturn, file->path, size);
    toReturn[size - 1] = '\0';
    return toReturn;
}

/**
 * Creates a source file reference for the given source file index.
 *
 * @param self the generified parser object
 * @param file the file index
 * @return the source file reference
 */
static inline struct dwarf_sourceFile dwarf5_getFileName(struct dwarf_parser* self, uint64_t file) {
    struct fileAttribute* filePtr = &self->specific.v5.files.content[file];
    return (struct dwarf_sourceFile) {
        dwarf5_constructFileName(filePtr, &self->specific.v5.directories),
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
    const uint8_t addressSize = *((uint8_t*) (self->debugLine.content + (*counter)++));
    const uint8_t segmentSelectorSize = *((uint8_t*) (self->debugLine.content + (*counter)++));
    (void) addressSize;
    (void) segmentSelectorSize;

    uint64_t headerLength;
    if (self->bit64) {
        headerLength = *((uint64_t*) (self->debugLine.content + (*counter)));
        *counter += 8;
    } else {
        headerLength = *((uint32_t*) (self->debugLine.content + (*counter)));
        *counter += 4;
    }
    (void) headerLength;

    self->minimumInstructionLength        = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->maximumOperationsPerInstruction = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->defaultIsStmt                   = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->lineBase                        = *((int8_t*)  (self->debugLine.content + (*counter)++));
    self->lineRange                       = *((uint8_t*) (self->debugLine.content + (*counter)++));
    self->opCodeBase                      = *((uint8_t*) (self->debugLine.content + (*counter)++));

    vector_uint8_reserve(&self->stdOpcodeLengths, self->opCodeBase - 1);
    for (uint8_t i = 1; i < self->opCodeBase; ++i) {
        vector_uint8_push_back(&self->stdOpcodeLengths, *((uint8_t*) (self->debugLine.content + (*counter)++)));
    }

    optional_vector_fileAttribute_t maybeDirs = dwarf5_parseFileAttributes(self, counter);
    if (!maybeDirs.has_value) {
        return false;
    }
    self->specific.v5.directories = maybeDirs.value;

    optional_vector_fileAttribute_t maybeFiles = dwarf5_parseFileAttributes(self, counter);
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
static inline void dwarf5_parser_destroy(struct dwarf_parser* self) {
    vector_fileAttribute_destroy(&self->specific.v5.files);
    vector_fileAttribute_destroy(&self->specific.v5.directories);
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
