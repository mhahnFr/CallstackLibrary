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

#include <map>
#include <memory>
#include <tuple>

extern "C" {
#include "../objectFile.h"
}

/**
 * This class acts as a C++ wrapper around the object file structure.
 */
class ObjectFile {
    /** The object file structure.                                     */
    objectFile self;
    /** A mapping of the contained functions to their start addresses. */
    std::map<uint64_t, function, std::greater<uint64_t>> functions;
    
public:
    /**
     * Adds the given function structure to this object file representation.
     *
     * @param function the function structure to be added
     */
    inline void addFunction(function && function) {
        functions.emplace(std::make_pair(function.startAddress, function));
    }
    
    inline operator objectFile *() {
        return &self;
    }
    
    inline objectFile * operator->() {
        return *this;
    }
};

objectFile * objectFile_new() {
    try {
        auto toReturn = new ObjectFile();
        
        objectFile_create(*toReturn);
        (*toReturn)->priv = toReturn;
        
        return *toReturn;
    } catch (...) {
        return nullptr;
    }
}

void objectFile_addFunction(objectFile * self, function * func) {
    reinterpret_cast<ObjectFile *>(self)->addFunction(function(*func));
    function_delete(func);
}

void objectFile_destroy(objectFile *) {}

void objectFile_delete(objectFile * self) {
    delete reinterpret_cast<ObjectFile *>(self->priv);
}
