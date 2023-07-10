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

#include "machoFile.h"
#include "machoFileInternal.h"

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
}

char * machoFile_addr2String(struct binaryFile * me, Dl_info * info) {
    struct machoFile * self = machoFileOrNull(me);
    if (self == NULL) {
        return NULL;
    }
    if (!self->_.parsed && !machoFile_parseFile(self, info)) {
        return NULL;
    }
    
    // TODO: Implement
    (void) info;
    return NULL;
}

void machoFile_destroy(struct binaryFile * self) {
    (void) self;
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
