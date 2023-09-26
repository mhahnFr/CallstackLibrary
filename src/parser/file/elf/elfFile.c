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

#include <stdlib.h>

#include "elfFile.h"

struct elfFile * elfFile_new(void) {
    struct elfFile * toReturn = malloc(sizeof(struct elfFile));
    
    if (toReturn != NULL) {
        elfFile_create(toReturn);
    }
    return toReturn;
}

void elfFile_create(struct elfFile * self) {
    binaryFile_create(&self->_);
    
    self->_.type     = ELF_FILE;
    self->_.concrete = self;
    
    self->_.addr2String = &elfFile_addr2String;
    self->_.destroy     = &elfFile_destroy;
    self->_.delete      = &elfFile_delete;
}

bool elfFile_addr2String(struct binaryFile *      self,
                                Dl_info *         info,
                                void *            address,
                         struct callstack_frame * frame) {
    (void) self;
    (void) info;
    (void) address;
    (void) frame;
    
    return NULL;
}

void elfFile_destroy(struct binaryFile * self) {
    (void) self;
}

void elfFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
