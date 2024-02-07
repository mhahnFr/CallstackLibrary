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

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "../objectFile.h"

/**
 * This class acts as a C++ wrapper around the object file structure.
 */
class ObjectFile {
    /** The object file structure.                                     */
    objectFile self;
    std::vector<function> ownFunctions;
    std::map<uint64_t, dwarf_lineInfo, std::greater<uint64_t>> lineInfos;
    std::optional<std::string> fullName;
    
    static inline void dwarfLineCallback(dwarf_lineInfo info, va_list args) {
        ObjectFile* self = reinterpret_cast<ObjectFile*>(va_arg(args, void*));
        
        self->lineInfos.emplace(std::make_pair(info.address, info));
    }
    
    inline auto parse() -> bool {
        const auto result = objectFile_parse(&self, dwarfLineCallback, this);
        if (!result) {
            for (auto& elem : ownFunctions) {
                function_destroy(&elem);
            }
            ownFunctions.clear();
        }
        return result;
    }
    
    inline auto getName() -> const std::string& {
        if (!fullName.has_value()) {
            fullName = std::string(self.directory) + std::string(self.sourceFile);
        }
        return *fullName;
    }
    
public:
    ~ObjectFile() {
        objectFile_destroy(&self);
        for (auto& elem : ownFunctions) {
            function_destroy(&elem);
        }
        for (auto& [address, info] : lineInfos) {
            dwarf_lineInfo_destroy(&info);
        }
    }
    
    inline auto parseBuffer(void* buffer) -> bool {
        const auto result = objectFile_parseWithBuffer(&self, buffer, dwarfLineCallback, this);
        if (!result) {
            for (auto& elem : ownFunctions) {
                function_destroy(&elem);
            }
            ownFunctions.clear();
        }
        return result;
    }
    
    inline void addOwnFunction(function&& function) {
        ownFunctions.emplace_back(function);
    }
    
    inline auto getDebugInfo(const function& function, uint64_t address) {
        optional_debugInfo_t toReturn = { .has_value = false };
        toReturn = {
            true, {
                .function = function,
                .sourceFileInfo.has_value = false
            }
        };
        
        if (!self.parsed) {
            if (!(self.parsed = parse())) {
                return toReturn;
            }
        }
        const auto ownFunction = std::find_if(ownFunctions.cbegin(), ownFunctions.cend(), [&](const auto value) {
            return std::string(value.linkedName) == std::string(function.linkedName);
        });
        if (ownFunction == ownFunctions.cend()) {
            return toReturn;
        }
        const uint64_t lineAddress = ownFunction->startAddress + address - function.startAddress;
        
        const auto closest = lineInfos.upper_bound(lineAddress);
        if (closest == lineInfos.end()) {
            return toReturn;
        }
        toReturn.value.sourceFileInfo = (optional_sourceFileInfo_t) {
            true, {
                closest->second.line,
                closest->second.column,
                closest->second.fileName == nullptr ? getName().c_str() : closest->second.fileName
            }
        };
        
        return toReturn;
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

auto objectFile_parseBuffer(objectFile* self, void* buffer) -> bool {
    return reinterpret_cast<ObjectFile*>(self->priv)->parseBuffer(buffer);
}

void objectFile_addOwnFunction(objectFile* self, function func) {
    reinterpret_cast<ObjectFile*>(self->priv)->addOwnFunction(std::move(func));
}

auto objectFile_getDebugInfo(objectFile* me, uint64_t address, function function) -> optional_debugInfo_t {
    return reinterpret_cast<ObjectFile*>(me->priv)->getDebugInfo(function, address);
}

void objectFile_destroy(objectFile * self) {
    std::free(self->name);
    std::free(self->directory);
    std::free(self->sourceFile);
}

void objectFile_delete(objectFile * self) {
    delete reinterpret_cast<ObjectFile *>(self->priv);
}
