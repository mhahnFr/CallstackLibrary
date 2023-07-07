/*
 * Callstack Library - A library creating human readable call stacks.
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

#include "llvmFile.h"
#include "llvmFileInternal.h"

struct llvmFile * llvmFile_new(void)  {
    struct llvmFile * toReturn = malloc(sizeof(struct llvmFile));
    
    if (toReturn != NULL) {
        llvmFile_create(toReturn);
    }
    return toReturn;
}

void llvmFile_create(struct llvmFile * self) {
    binaryFile_create(&self->_);
    
    self->_.type     = LLVM_FILE;
    self->_.concrete = self;
    
    self->_.addr2String = &llvmFile_addr2String;
    self->_.destroy     = &llvmFile_destroy;
    self->_.delete      = &llvmFile_delete;
}

char * llvmFile_addr2String(struct binaryFile * me, Dl_info * info) {
    struct llvmFile * self = llvmFileOrNull(me);
    if (self == NULL) {
        return NULL;
    }
    if (!self->_.parsed && !llvmFile_parseFile(self)) {
        return NULL;
    }
    
    // TODO: Implement
    (void) info;
    return NULL;
}

void llvmFile_destroy(struct binaryFile * self) {
    (void) self;
}

void llvmFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
