/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "machoFile.h"
#include "machoFileInternal.h"

#include "../../callstack_parserInternal.h"

struct machoFile * machoFile_new(void)  {
    struct machoFile * toReturn = malloc(sizeof(struct machoFile));
    
    if (toReturn != NULL) {
        machoFile_create(toReturn);
    }
    return toReturn;
}

void machoFile_create(struct machoFile * self) {
    binaryFile_create(&self->_);
    
    self->_.type     = MACHO_FILE;
    self->_.concrete = self;
    
    self->_.addr2String = &machoFile_addr2String;
    self->_.destroy     = &machoFile_destroy;
    self->_.delete      = &machoFile_delete;
    
    self->addressOffset = 0x0;
    self->objectFiles   = NULL;
    
    vector_uint64_t_create(&self->functionStarts);
}

/**
 * Caches the represented file from disk and parses it.
 *
 * @param self the Mach-O file object
 * @return whether the file was successfully read and parsed
 */
static inline bool machoFile_readAndParseFile(struct machoFile * self) {
    if (self->_.fileName == NULL) return false;
    
    struct stat fileStats;
    if (stat(self->_.fileName, &fileStats) != 0) {
        return false;
    }
    void * buffer = malloc(fileStats.st_size);
    if (buffer == NULL) {
        return false;
    }
    FILE * file = fopen(self->_.fileName, "r");
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    const bool toReturn = (off_t) count == fileStats.st_size && machoFile_parseFile(self, buffer);
    free(buffer);
    return toReturn;
}

bool machoFile_addr2String(struct binaryFile *      me,
                                  Dl_info *         info,
                                  void *            address,
                           struct callstack_frame * frame) {
    struct machoFile * self = machoFileOrNull(me);
    if (self == NULL) {
        return NULL;
    }
    if (!self->_.parsed &&
        !(self->_.parsed = machoFile_readAndParseFile(self))) {
        return NULL;
    }
    
    struct optional_funcFile result = machoFile_findFunction(self, info->dli_fbase, address);
    if (result.has_value) {
        // TODO: Parse DWARF data if available!
        char * name = result.value.first.linkedName;
        if (name != NULL) {
            if (*name == '_' || *name == '\1') {
                ++name;
            }
            name = callstack_parser_demangle(name);
            char * toReturn = NULL;
            asprintf(&toReturn, "%s + %td",
                     name,
                     (ptrdiff_t) (address - info->dli_fbase + self->addressOffset - result.value.first.startAddress));
            free(name);
            frame->function = toReturn;
            return true;
        }
    }
    return false;
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile * self = machoFileOrNull(me);
    if (self == NULL) {
        return;
    }
    
    for (struct objectFile * tmp = self->objectFiles; tmp != NULL;) {
        struct objectFile * n = tmp->next;
        objectFile_delete(tmp);
        tmp = n;
    }
    
    vector_uint64_t_destroy(&self->functionStarts);
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
