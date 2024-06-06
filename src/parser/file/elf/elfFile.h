/*
 * CallstackLibrary - Library creating human-readable call stacks.
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

#ifndef elfFile_h
#define elfFile_h

#include "../binaryFile.h"
#include "../debugInfo.h"
#include "../function.h"
#include "../lcs_section.h"

#include "../dwarf/dwarf_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents an ELF binary file.
 */
struct elfFile {
    /** The super part of this structure.                          */
    struct binaryFile _;

    /** The section corresponding to the .debug_line section.      */
    struct lcs_section debugLine,
    /** The section corresponding to the .debug_line_str section.  */
                       debugLineStr,
    /** The section corresponding to the .debug_str section.       */
                       debugStr;
    
    /** The pointer to the private part of this object.            */
    void* priv;
};

/**
 * Allocates a new ELF file structure.
 *
 * @return the allocated structure or `NULL` on error
 */
struct elfFile * elfFile_new(void);

/**
 * Initializes the given ELF file structure.
 *
 * @param self the structure to be initialized
 */
void elfFile_create(struct elfFile * self);

/**
 * Returns an ELF file structure from the given binary file structure
 * or `NULL`, if the binary file structure does not represent an ELF
 * file structure.
 *
 * @param self the binary file structure to be casted
 * @return the ELF file structure on success or `NULL`
 */
static inline struct elfFile* elfFileOrNull(struct binaryFile * self) {
    return self->type == ELF_FILE ? (struct elfFile*) self->concrete : NULL;
}

/**
 * Returns the optionally available debug information for the given address.
 *
 * @param self the elf file structure object
 * @param address the address to get debug information for
 * @return the optionally available debug information
 */
optional_debugInfo_t elfFile_getDebugInfo(struct elfFile* self, void* address);

/**
 * Loads the represented ELF file.
 *
 * @param self the elf file structure object
 * @return whether the loading was successful
 */
bool elfFile_loadFile(struct elfFile* self);

/**
 * Parses the given buffer into the given elf file structure object.
 *
 * @param self the elf file structure object
 * @param buffer the buffer to be parsed
 * @param cb the DWARF line callback function
 * @param args the payload to pass to the given callback
 * @return whether the parsing was successful
 */
bool elfFile_parseFile(struct elfFile* self, void* buffer, dwarf_line_callback cb, void* args);

/**
 * @brief Adds the given function to the given elf file structure object.
 *
 * If a function with the same address already exists, one of them is discarded.
 *
 * @param self the elf file structure object
 * @param f the function object to be added
 */
void elfFile_addFunction(struct elfFile* self, struct function f);

/**
 * Loads the debug information available for the given address into the given
 * callstack frame object.
 *
 * @param self the binary file object
 * @param address the address to get debug information for
 * @param frame the callstack frame object to store the information in
 * @return whether the callstack frame object was filled successfully
 */
bool elfFile_addr2String(struct binaryFile* self, void* address, struct callstack_frame* frame);

/**
 * Deinitializes the given binary file structure, if it is an ELF file structure.
 *
 * @param self the binary file structure to be deinitialized
 */
void elfFile_destroy(struct binaryFile * self);

/**
 * Deinitializes and `free`s the given binary file, if it is an ELF file structure.
 *
 * @param self the binary file structure to be deleted
 */
void elfFile_delete(struct binaryFile * self);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* elfFile_h */
