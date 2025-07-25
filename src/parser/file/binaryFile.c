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

#include <stddef.h>
#include <sys/stat.h>

#include "binaryFile.h"

#ifdef __APPLE__
# include "macho/machoFile.h"
# define LCS_MACHO
#elif defined(__linux__)
# include "elf/elfFile.h"
# define LCS_ELF
#endif

#ifdef LCS_MACHO
# define LCS_FILE_NAME machoFile
# define LCS_FILE_RAW(NAME) machoFile_##NAME
#elif defined(LCS_ELF)
# define LCS_FILE_NAME elfFile
# define LCS_FILE_RAW(NAME) elfFile_##NAME
#endif

/** The concrete binary file abstraction structure type.         */
typedef struct LCS_FILE_NAME ConcreteFile;
/** The concrete binary file abstraction structure pointer type. */
typedef ConcreteFile* Concrete;

/**
 * Calls the concrete implementation of the named function.
 *
 * @param self the instance object
 * @param NAME the name of the requested function
 * @param ... the arguments to be passed to the requested function
 */
#define LCS_FILE(self, NAME, ...) LCS_FILE_RAW(NAME)((Concrete) (self) __VA_OPT__(,) __VA_ARGS__)

struct binaryFile* binaryFile_new(const char* fileName, const void* startAddress) {
    Concrete tmp = LCS_FILE_RAW(new)();
    struct binaryFile* toReturn = &tmp->_;

    if (toReturn != NULL) {
        toReturn->fileName     = fileName;
        toReturn->startAddress = startAddress;
    }
    return toReturn;
}

bool binaryFile_addr2String(struct binaryFile* self, const void* address, struct callstack_frame* frame) {
    return LCS_FILE(self, addr2String, address, frame);
}

bool binaryFile_getFunctionInfo(struct binaryFile* self, const char* functionName, struct functionInfo* info) {
    return LCS_FILE(self, getFunctionInfo, functionName, info);
}

vector_pair_ptr_t binaryFile_getTLSRegions(struct binaryFile* self) {
    return LCS_FILE(self, getTLSRegions);
}

bool binaryFile_maybeParse(struct binaryFile* self) {
    return self->parsed || ((self->parsed = LCS_FILE(self, parse)));
}

void binaryFile_clearCaches(void) {
#ifdef LCS_MACHO
    machoFile_clearCaches();
#endif
}

bool binaryFile_isOutdated(const struct dwarf_sourceFile file) {
    if (file.fileName == NULL || file.timestamp == 0) {
        return false;
    }
    struct stat fileStats;
    if (stat(file.fileName, &fileStats) != 0) {
        return false;
    }
    if (fileStats.st_mtime != (time_t) file.timestamp) {
        return true;
    }
    if (file.size != 0 && fileStats.st_size != (off_t) file.size) {
        return true;
    }
    return false;
}

void binaryFile_destroy(struct binaryFile* self) {
    LCS_FILE(self, destroy);
    vector_destroy(&self->regions);
}

void binaryFile_delete(struct binaryFile* self) {
    LCS_FILE(self, delete);
}
