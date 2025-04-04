/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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
#include "../lcs_section.h"
#include "../vector_function.h"

#include "../dwarf/vector_dwarf_lineInfo.h"

/**
 * This structure represents an ELF binary file.
 */
struct elfFile {
    /** The super part of this structure.                            */
    struct binaryFile _;

    /** The section corresponding to the .debug_line section.        */
    struct lcs_section debugLine,
    /** The section corresponding to the .debug_line_str section.    */
                       debugLineStr,
    /** The section corresponding to the .debug_str section.         */
                       debugStr,
    /** The section corresponding to the @c .debug_info section.     */
                       debugInfo,
    /** The section corresponding to the @c .debug_abbrev section.   */
                       debugAbbrev,
    /** The section corresponding to the @c .debug_str_offsets.      */
                       debugStrOffsets;
    
    /** The functions found in the represented ELF file.             */
    vector_function_t functions;
    /* The DWARF line information found in the represented ELF file. */
    vector_dwarfLineInfo_t lineInfos;
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
 * Loads the ELF file represented by the given abstraction object.
 *
 * @param self the ELF file abstraction object
 * @return whether the ELF file was loaded successfully
 */
bool elfFile_parse(struct elfFile* self);

/**
 * Loads the debug information available for the given address into the given
 * callstack frame object.
 *
 * @param self the binary file object
 * @param address the address to get debug information for
 * @param frame the callstack frame object to store the information in
 * @return whether the callstack frame object was filled successfully
 */
bool elfFile_addr2String(struct elfFile* self, void* address, struct callstack_frame* frame);

/**
 * Loads the function information for the function of the given name.
 *
 * @param self the ELF file abstraction structure to search in
 * @param functionName the name of the function to load the information
 * @param info the @c functionInfo structure to fill in
 * @return whether the function was found
 */
bool elfFile_getFunctionInfo(struct elfFile* self, const char* functionName, struct functionInfo* info);

/**
 * Deinitializes the given binary file structure, if it is an ELF file structure.
 *
 * @param self the binary file structure to be deinitialized
 */
void elfFile_destroy(struct elfFile* self);

/**
 * Deinitializes and `free`s the given binary file, if it is an ELF file structure.
 *
 * @param self the binary file structure to be deleted
 */
void elfFile_delete(struct elfFile* self);

#endif /* elfFile_h */
