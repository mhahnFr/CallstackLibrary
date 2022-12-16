/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022  mhahnFr
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

#ifndef llvm_file_h
#define llvm_file_h

#include <stdlib.h>

struct llvm_file {
    char * fileName;
    // ...
};

struct llvm_file * llvm_file_new(char * fileName);

static inline void llvm_file_create(struct llvm_file * self, char * fileName) {
    self->fileName = fileName;
}

static inline void llvm_file_destroy(struct llvm_file * self) {
    (void) self;
}

static inline void llvm_file_delete(struct llvm_file * self) {
    llvm_file_destroy(self);
    free(self);
}

#endif /* llvm_file_h */
