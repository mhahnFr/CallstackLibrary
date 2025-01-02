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
 #include "macho/machoFile.h"
 #define LCS_MACHO
#elif defined(__linux__)
 #include "elf/elfFile.h"
 #define LCS_ELF
#endif

struct binaryFile* binaryFile_new(const char* fileName, const void* startAddress) {
    struct binaryFile * toReturn;
    
#ifdef LCS_MACHO
    struct machoFile* tmp = machoFile_new();
    toReturn = tmp == NULL ? NULL : &tmp->_;
#elif defined(LCS_ELF)
    struct elfFile * tmp = elfFile_new();
    toReturn = tmp == NULL ? NULL : &tmp->_;
#else
    toReturn = NULL;
#endif
    
    if (toReturn != NULL) {
        toReturn->fileName     = fileName;
        toReturn->startAddress = startAddress;
    }
    return toReturn;
}

bool binaryFile_addr2String(struct binaryFile* self, void* address, struct callstack_frame* frame) {
    bool result = false;
#ifdef LCS_MACHO
    result = machoFile_addr2String((struct machoFile*) self, address, frame);
#elif defined(LCS_ELF)
    result = elfFile_addr2String((struct elfFile*) self, address, frame);
#endif
    return result;
}

bool binaryFile_getFunctionInfo(struct binaryFile* self, const char* functionName, struct functionInfo* info) {
    bool result = false;

#ifdef LCS_MACHO
    result = machoFile_getFunctionInfo((struct machoFile*) self, functionName, info);
#elif defined(LCS_ELF)
    result = elfFile_getFunctionInfo((struct elfFile*) self, functionName, info);
#endif

    return result;
}

void binaryFile_clearCaches(void) {
#ifdef LCS_MACHO
    machoFile_clearCaches();
#endif
}

bool binaryFile_isOutdated(struct dwarf_sourceFile file) {
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
#ifdef LCS_MACHO
    machoFile_destroy((struct machoFile*) self);
#elif defined(LCS_ELF)
    elfFile_destroy((struct elfFile*) self);
#endif
}

void binaryFile_delete(struct binaryFile* self) {
#ifdef LCS_MACHO
    machoFile_delete((struct machoFile*) self);
#elif defined(LCS_ELF)
    elfFile_delete((struct elfFile*) self);
#endif
}
