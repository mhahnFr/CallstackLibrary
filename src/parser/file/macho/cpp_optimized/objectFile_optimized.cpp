/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
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
    /** The object file structure.                               */
    objectFile self;
    /** The functions inside the represented object file.        */
    std::vector<function> ownFunctions;
    /** Mapping of the DWARF line info entries to their address. */
    std::map<uint64_t, dwarf_lineInfo, std::greater<uint64_t>> lineInfos;
    /** The full name of the represented source file.            */
    std::optional<std::string> fullName;
    
    /**
     * The callback function adding the given DWARF line entry to the object file object passed
     * via the variadic argument list.
     *
     * @param info the DWAF line info entry
     * @param args the arguments - should contain the object file object as first argument
     */
    static inline void dwarfLineCallback(dwarf_lineInfo info, void* args) {
        ObjectFile* self = reinterpret_cast<ObjectFile*>(args);
        
        self->lineInfos.emplace(std::make_pair(info.address, info));
    }
    
    /**
     * Parses the represented object file.
     *
     * @return whether the parsing was successful
     */
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
    
    /**
     * Returns the full name of the represented source file.
     *
     * @return the full source file name
     */
    inline auto getName() -> const std::string& {
        if (!fullName.has_value()) {
            if (self.directory == nullptr || self.sourceFile == nullptr) {
                fullName = "";
            } else {
                fullName = std::string(self.directory) + std::string(self.sourceFile);
            }
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
    
    /**
     * Parses the given buffer as represented object file.
     *
     * @param buffer the buffer to be parsed
     * @return whether the parsing was successful
     */
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
    
    /**
     * Adds the given function to this object file object.
     *
     * @param function the function to be added
     */
    inline void addOwnFunction(function&& function) {
        ownFunctions.emplace_back(function);
    }
    
    /**
     * Extracts the available debug information for the given address inside the given function.
     *
     * @param function the function inside the address is
     * @param address the address
     * @return the optionally deducted debug information
     */
    inline auto getDebugInfo(const function& function, uint64_t address) {
        optional_debugInfo_t toReturn = { .has_value = false };
        
        if (!self.parsed) {
            if (!(self.parsed = parse())) {
                return toReturn;
            }
        }
        uint64_t lineAddress;
        if (self.isDsymBundle) {
            lineAddress = address;
        } else {
            const auto ownFunction = std::find_if(ownFunctions.cbegin(), ownFunctions.cend(), [&](const auto& value) {
                return std::string(value.linkedName) == std::string(function.linkedName);
            });
            if (ownFunction == ownFunctions.cend()) {
                return toReturn;
            }
            lineAddress = ownFunction->startAddress + address - function.startAddress;
        }
        
        const auto closest = lineInfos.upper_bound(lineAddress);
        if (closest == lineInfos.end()) {
            return toReturn;
        }
        toReturn = {
            true, {
                function, {
                    true, {
                        closest->second.line,
                        closest->second.column,
                        closest->second.fileName == nullptr ? getName().c_str() : closest->second.fileName
                    }
                }
            }
        };
        return toReturn;
    }
    
    /**
     * @brief Returns the UUID of the represented Mach-O file.
     *
     * Parses the represented source file if it has not already been parsed.
     *
     * @return the UUID of the represented Mach-O file
     */
    inline auto getUUID() -> uint8_t* {
        if (!self.parsed) {
            self.parsed = parse();
        }
        return self.uuid;
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

auto objectFile_getUUID(objectFile* me) -> uint8_t* {
    return reinterpret_cast<ObjectFile*>(me->priv)->getUUID();
}

void objectFile_destroy(objectFile * self) {
    std::free(self->name);
    std::free(self->directory);
    std::free(self->sourceFile);
}

void objectFile_delete(objectFile * self) {
    delete reinterpret_cast<ObjectFile *>(self->priv);
}
