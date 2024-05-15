/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include "../machoFile.h"

#include <cstring>
#include <map>

/**
 * This class wraps around the Mach-O file representation.
 */
class MachoFile {
    /** The represented Mach-O file representation.                                   */
    machoFile self;

    /** Mapping of the functions inside the represented Mach-O file to their address. */
    std::map<uint64_t, std::pair<function, objectFile*>, std::greater<uint64_t>> functions;
    
public:
    /**
     * Constructs a Mach-O file representation using the given file name.
     *
     * @param fileName the file name of the represented Mach-O file
     */
    inline MachoFile(const char* fileName) {
        machoFile_create(*this, fileName);
        self.priv = this;
    }
    
    inline ~MachoFile() noexcept {
        machoFile_destroy(&self._);
        
        for (auto& [address, funcFile] : functions) {
            function_destroy(&funcFile.first);
        }
    }
    
    /**
     * Adds the given function / object file pair to this Mach-O file representation.
     *
     * @param function the function / object file pair to be added
     */
    inline void addFunction(pair_funcFile_t&& function) {
        auto it = functions.find(function.first.startAddress);
        if (it == functions.end()) {
            functions.emplace(std::make_pair(function.first.startAddress, dc4c::to_cpp(function)));
        } else {
            if (function.second != nullptr) {
                function_destroy(&it->second.first);
                it->second = dc4c::to_cpp(function);
            } else {
                function_destroy(&function.first);
            }
        }
    }
    
    /**
     * Extracts the available debug information for the given absolute address.
     *
     * @param addr the absolute instruction address
     * @return the optionally deducted debug information
     */
    inline auto getDebugInfo(void* addr) -> optional_debugInfo_t {
        const uint64_t address = (reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(self._.startAddress))
                               + (self.inMemory ? self.text_vmaddr : self.addressOffset);
        auto it = functions.lower_bound(address);
        if (it == functions.end()) {
            return { .has_value = false };
        }
        optional_debugInfo_t info = { .has_value = false };
        if (machoFile_getDSYMBundle(&self) != nullptr && memcmp(objectFile_getUUID(self.dSYMFile.file), self.uuid, 16) == 0) {
            info = objectFile_getDebugInfo(self.dSYMFile.file, address, it->second.first);
            if (info.has_value) {
                return info;
            }
        }
        if (it->second.second != nullptr) {
            info = objectFile_getDebugInfo(it->second.second, address, it->second.first);
        }
        if (!info.has_value) {
            info = {
                true, {
                    it->second.first,
                    .sourceFileInfo.has_value = false
                }
            };
        }
        return info;
    }
    
    inline operator machoFile*() {
        return &self;
    }
    
    inline auto operator->() -> machoFile* {
        return *this;
    }
};

auto machoFile_new(const char* fileName) -> machoFile* {
    try {
        return *(new MachoFile(fileName));
    } catch (...) {
        return nullptr;
    }
}

void machoFile_addFunction(machoFile* me, pair_funcFile_t function) {
    reinterpret_cast<MachoFile*>(me->priv)->addFunction(std::move(function));
}

auto machoFile_getDebugInfo(machoFile* me, void* address) -> optional_debugInfo_t {
    return reinterpret_cast<MachoFile*>(me->priv)->getDebugInfo(address);
}

void machoFile_destroy(binaryFile* me) {
    auto self = reinterpret_cast<machoFile*>(me->concrete);
    if (self->dSYMFile.file != nullptr) {
        objectFile_delete(self->dSYMFile.file);
    }
}

void machoFile_delete(binaryFile* me) {
    auto tmp = machoFileOrNull(reinterpret_cast<binaryFile*>(me->concrete));
    if (tmp == nullptr) {
        return;
    }
    delete reinterpret_cast<MachoFile*>(tmp->priv);
}
