/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef objectFile_h
#define objectFile_h

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "../debugInfo.h"
#include "../function.h"
#include "../lcs_section.h"
#include "../optional_function.h"

#include "../dwarf/dwarf_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents an object file.
 */
struct objectFile {
    /** The name of the corresponding source file.              */
    char * sourceFile;
    /** The directory of the corresponding source file.         */
    char * directory;
    /** The full path of the represented object file.           */
    char * name;
    
    /** The timestamp of the last modification.                 */
    time_t lastModified;
    
    /** The UUID of the represented Mach-O object file.         */
    uint8_t uuid[16];
    
    struct lcs_section debugLine;
    struct lcs_section debugLineStr;
    struct lcs_section debugStr;

    /** Whether the file was successfully parsed.               */
    bool parsed;
    /** Whether the represented file is part of a .dSYM bundle. */
    bool isDsymBundle;
    
    /** A pointer to the underlying object.                     */
    void * priv;
    /** Pointer to the next element in a list.                  */
    struct objectFile * next;
};

/**
 * Allocates and initializes a new object file structure.
 *
 * @return the allocated object or `NULL` on error
 */
struct objectFile * objectFile_new(void);

/**
 * Initializes the given object file structure.
 *
 * @param self the object file structure to be initialized
 */
static inline void objectFile_create(struct objectFile * self) {
    self->sourceFile   = NULL;
    self->directory    = NULL;
    self->name         = NULL;
    self->priv         = NULL;
    self->next         = NULL;
    self->lastModified = 0;
    self->parsed       = false;
    self->isDsymBundle = false;
    
    memset(self->uuid, 0, 16);

    lcs_section_create(&self->debugLine);
    lcs_section_create(&self->debugLineStr);
    lcs_section_create(&self->debugStr);
}

/**
 * Adds the given function to the given object file.
 *
 * @param self the object file object
 * @param function the function to be added
 */
void objectFile_addOwnFunction(struct objectFile* self, struct function function);

/**
 * Parses the given buffer into the given object file object.
 *
 * @param self the object file object
 * @param buffer the Mach-O buffer to be parsed
 * @return whether the parsing was successful
 */
bool objectFile_parseBuffer(struct objectFile* self, void* buffer);

/**
 * @brief Parses the Mach-O file represented by the given object file object.
 *
 * The DWARF line information is extracted and for every line entry the given
 * callback is called with the additionally given arguments.
 *
 * @param self the object file object to be parsed
 * @param cb the callback to be called for every DWARF line entry
 * @return whether the parsing was successful
 */
bool objectFile_parse(struct objectFile* self, dwarf_line_callback cb, void* args);

/**
 * @brief Parses the given buffer into the given object file object.
 *
 * The DWARF line information is extracted and for every line entry the given
 * callback is called with the additionally given arguments.
 *
 * @param self the object file object
 * @param buffer the Mach-O buffer
 * @param cb the callback to be called for DWARF line entries
 * @return whether the buffer was parsed successfully
 */
bool objectFile_parseWithBuffer(struct objectFile* self, void* buffer, dwarf_line_callback cb, void* args);

/**
 * @brief Returns the UUID of the given object file object.
 *
 * Prefer using this method since the object file needs to be
 * parsed before the UUID is available.
 *
 * @param self the object file object
 * @return the Mach-O UUID
 */
uint8_t* objectFile_getUUID(struct objectFile* self);

/**
 * @brief Extracts the debug information for the given address inside the given function
 * from the given object file object.
 *
 * If no information could be deducted, an empty optional is returned.
 *
 * @param self the object file object
 * @param address the address inside the function
 * @param function the function
 * @return the optionally deducted debug information
 */
optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* self, uint64_t address, struct function function);

/**
 * Deinitializes the given object file structure.
 *
 * @param self the object file structure to be deinitialized
 */
void objectFile_destroy(struct objectFile * self);

/**
 * Deinitializes and `free`s the given object file structure.
 *
 * @param self the object file structure to be deleted
 */
void objectFile_delete(struct objectFile * self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* objectFile_h */
