/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef objectFile_h
#define objectFile_h

#include <stdarg.h>

#include "function.h"
#include "OptionalFunction.h"

/**
 * This structure represents an object file.
 */
struct objectFile {
    /** The name of the corresponding source file.      */
    char * sourceFile;
    /** The directory of the corresponding source file. */
    char * directory;
    /** The full path of the represented object file.   */
    char * name;
    
    /** A pointer to the underlying object.             */
    void * priv;
    /** Pointer to the next element in a list.          */
    struct objectFile * next;
};

/**
 * Allocates and initializes a new object file structure.
 *
 * @return the allocated object or `NULL` on error
 */
struct objectFile * objectFile_new(void);

/**
 * Initializes the given object file structure.
 *
 * @param self the object file structure to be initialized
 */
static inline void objectFile_create(struct objectFile * self) {
    self->sourceFile = NULL;
    self->directory  = NULL;
    self->name       = NULL;
    self->priv       = NULL;
    self->next       = NULL;
}

/**
 * Adds the given function structure to the given object file structure.
 *
 * @param self the object file structure to add to
 * @param function the function structure to be added
 */
void objectFile_addFunction(struct objectFile * self,
                            struct function     function);

/**
 * Searches and returns the function in which the given address is in.
 *
 * @param self the object file instance
 * @param address the address whose function to be searched
 * @return the function if found
 */
optional_function_t objectFile_findFunction(struct objectFile * self, uint64_t address);

/**
 * @brief Invokes the given function for each function object inside the given object file.
 *
 * The additional parameters are passed as `va_list` to the given function.
 *
 * @param self the object file instance
 * @param func the function to be invoked
 */
void objectFile_functionsForEach(struct objectFile * self, void (*func)(struct function *, va_list), ...);

/**
 * Deinitializes the given object file structure.
 *
 * @param self the object file structure to be deinitialized
 */
void objectFile_destroy(struct objectFile * self);

/**
 * Deinitializes and `free`s the given object file structure.
 *
 * @param self the object file structure to be deleted
 */
void objectFile_delete(struct objectFile * self);

#endif /* objectFile_h */
