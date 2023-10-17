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
    ~ObjectFile() {
        objectFile_destroy(&self);
        for (auto & elem : functions) {
            function_destroy(&elem.second);
        }
    }
    
    /**
     * Adds the given function structure to this object file representation.
     *
     * @param function the function structure to be added
     */
    inline void addFunction(function && function) {
        functions.emplace(std::make_pair(function.startAddress, function));
    }
    
    /**
     * Searches and returns the function in which the given address is in.
     *
     * @param address the address whose function to be searched
     * @return the function if found
     */
    inline auto findFunction(uint64_t address) -> optional_function_t {
        auto result = functions.lower_bound(address);
        optional_function_t toReturn = { .has_value = false };
        
        if (result == functions.end() || address > result->second.endAddress) {
            return toReturn;
        }
        toReturn.has_value = true;
        toReturn.value     = result->second;
        return toReturn;
    }
    
    /**
     * Performs the given function with the given arguments for each function
     * inside this object file.
     *
     * @param func the function to be executed
     * @param args the additional arguments to be passed
     */
    inline void functionsForEach(void (*func)(function *, va_list), va_list & args) {
        for (auto & elem : functions) {
            va_list copy;
            va_copy(copy, args);
            func(std::addressof(elem.second), copy);
            va_end(copy);
        }
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

void objectFile_addFunction(objectFile * self, function func) {
    reinterpret_cast<ObjectFile *>(self)->addFunction(std::move(func));
}

auto objectFile_findFunction(objectFile * me, uint64_t address) -> optional_function_t {
    return reinterpret_cast<ObjectFile *>(me)->findFunction(address);
}

void objectFile_functionsForEach(objectFile * me, void (*func)(function *, va_list), ...) {
    va_list list;
    va_start(list, func);
    reinterpret_cast<ObjectFile *>(me)->functionsForEach(func, list);
    va_end(list);
}

void objectFile_destroy(objectFile * self) {
    std::free(self->name);
    std::free(self->directory);
    std::free(self->sourceFile);
}

void objectFile_delete(objectFile * self) {
    delete reinterpret_cast<ObjectFile *>(self->priv);
}
