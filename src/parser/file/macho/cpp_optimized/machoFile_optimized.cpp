/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include "../machoFile.h"

#include <forward_list>
#include <map>
#include <vector>

class MachoFile {
    machoFile self;
    
    std::vector<objectFile*> objectFiles;
    std::forward_list<function> functionStorage;
    std::map<uint64_t, std::pair<function*, objectFile*>, std::greater<uint64_t>> functions;
    
public:
    inline MachoFile(const char* fileName) {
        machoFile_create(*this, fileName);
        self.priv = this;
    }
    
    inline ~MachoFile() noexcept {
        machoFile_destroy(&self._);
        for (auto& file : objectFiles) {
            objectFile_delete(file);
        }
        for (auto& func : functionStorage) {
            function_destroy(&func);
        }
    }
    
    inline void addFunction(function&& function) {
        auto it = functionStorage.insert_after(functionStorage.before_begin(), function);
        functions.emplace(std::make_pair(it->startAddress, std::make_pair(&(*it), nullptr)));
    }
    
    constexpr inline void addObjectFile(objectFile* file) {
        objectFiles.push_back(file);
        objectFile_functionsForEach(file, [](function* it, va_list args) {
            MachoFile*  self = reinterpret_cast<MachoFile*> (va_arg(args, void*));
            objectFile* file = reinterpret_cast<objectFile*>(va_arg(args, void*));
            
            self->functions.insert_or_assign(it->startAddress, std::make_pair(it, file));
        }, this, file);
    }
    
    inline auto getDebugInfo(void* addr) -> optional_debugInfo_t {
        const uint64_t address = (reinterpret_cast<uint64_t>(addr) - reinterpret_cast<uint64_t>(self._.startAddress)) 
                               + (self.inMemory ? self.text_vmaddr : self.addressOffset);
        auto it = functions.lower_bound(address);
        if (it == functions.end()) {
            return { .has_value = false };
        }
        optional_debugInfo_t info = { .has_value = false };
        if (it->second.second != nullptr) {
            info = objectFile_getDebugInfo(it->second.second, address, *it->second.first);
        }
        if (!info.has_value) {
            info = {
                true, {
                    *it->second.first,
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

void machoFile_addFunction(machoFile* me, function function) {
    reinterpret_cast<MachoFile*>(me->priv)->addFunction(std::move(function));
}

void machoFile_addObjectFile(machoFile* me, objectFile* file) {
    reinterpret_cast<MachoFile*>(me->priv)->addObjectFile(file);
}

auto machoFile_getDebugInfo(machoFile* me, void* address) -> optional_debugInfo_t {
    return reinterpret_cast<MachoFile*>(me->priv)->getDebugInfo(address);
}

void machoFile_destroy(binaryFile*) {
    // Nothing to do so far.    - mhahnFr
}

void machoFile_delete(binaryFile* me) {
    auto tmp = machoFileOrNull(reinterpret_cast<binaryFile*>(me->concrete));
    if (tmp == nullptr) {
        return;
    }
    delete reinterpret_cast<MachoFile*>(tmp->priv);
}
