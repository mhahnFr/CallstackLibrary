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

#ifndef binaryFile_h
#define binaryFile_h

#include <dlfcn.h>
#include <stdbool.h>

enum binaryFileType {
    LLVM_FILE,
    GNU_FILE
};

struct binaryFile {
    enum binaryFileType type;
    void * concrete;
    
    char * fileName;
    bool   parsed;
    
    struct binaryFile * next;
    
    char * (*addr2String)(struct binaryFile *, Dl_info *);
    void   (*destroy)    (struct binaryFile *);
    void   (*delete)     (struct binaryFile *);
};

struct binaryFile * binaryFile_new(char * fileName);
void binaryFile_create(struct binaryFile * self);

#endif /* binaryFile_h */
