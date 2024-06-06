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

#include <map>

#include "../elfFile.h"

#include "../../loader.h"

/**
 * This class represents an ELF file.
 */
class ElfFile {
    /** The shared part of the ELF file abstraction.             */
    elfFile self;

    /** Mapping the address to its appropriate line table entry. */
    std::map<uint64_t, dwarf_lineInfo, std::greater<uint64_t>> lineInfos;
    /** Mapping the address to its appropriate function.         */
    std::map<uint64_t, function, std::greater<uint64_t>> functions;

public:
    inline ElfFile() {
        elfFile_create(*this);
        self.priv = this;
    }

    inline ~ElfFile() {
        elfFile_destroy(&self._);
        for (auto& [_, info] : lineInfos) {
            dwarf_lineInfo_destroy(&info);
        }
        for (auto& [_, function] : functions) {
            function_destroy(&function);
        }
    }

    /**
     * @brief Adds the given function object.
     *
     * If a function object with the same address exists, the given function is
     * discarded.
     *
     * @param f the function object to be added
     */
    inline void addFunction(const function& f) {
        if (functions.find(f.startAddress) == functions.end()) {
            functions.emplace(std::make_pair(f.startAddress, f));
        } else {
            function_destroy(&const_cast<function&>(f));
        }
    }

    /**
     * Loads and parses the represented binary file.
     *
     * @return whether the loading and parsing was successful
     */
    inline auto loadFile() -> bool {
        return loader_loadFileAndExecute(self._.fileName, {
            .parseFunc = [](auto arg, auto buffer) {
                auto self = reinterpret_cast<ElfFile*>(arg);

                return elfFile_parseFile(*self, buffer, [](auto info, auto args) {
                    auto self = reinterpret_cast<ElfFile*>(args);

                    if (self->lineInfos.find(info.address) == self->lineInfos.end()) {
                        self->lineInfos.emplace(std::make_pair(info.address, info));
                    } else {
                        dwarf_lineInfo_destroy(&info);
                    }
                }, self);
            }
        }, false, this);
    }

    /**
     * Retrieves the debug information available for the given address.
     *
     * @param address the address to find debug information for
     * @return the optionally found debug information
     */
    inline auto getDebugInfo(void* address) -> optional_debugInfo_t {
        optional_debugInfo_t info;
        info.has_value = false;

        const uint64_t translated = reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(self._.startAddress);
        auto it = functions.lower_bound(translated);
        if (it == functions.end() || it->first > translated || it->first + it->second.length < translated) {
            return info;
        }
        info.has_value = true;
        info.value.function = it->second;
        info.value.sourceFileInfo.has_value = false;

        auto lineIt = lineInfos.upper_bound(translated);
        if (lineIt == lineInfos.end() || lineIt->first <= it->first) {
            return info;
        }
        info.value.sourceFileInfo = {
            true, {
                lineIt->second.line,
                lineIt->second.column,
                lineIt->second.sourceFile.fileName,
                binaryFile_isOutdated(lineIt->second.sourceFile)
            }
        };
        return info;
    }

    constexpr inline operator elfFile*() {
        return &self;
    }

    constexpr inline auto operator->() -> elfFile* {
        return *this;
    }
};

elfFile* elfFile_new() {
    try {
        return *new ElfFile();
    } catch (...) {
        return nullptr;
    }
}

void elfFile_addFunction(elfFile* self, function f) {
    reinterpret_cast<ElfFile*>(self->priv)->addFunction(f);
}

bool elfFile_loadFile(elfFile* self) {
    return reinterpret_cast<ElfFile*>(self->priv)->loadFile();
}

optional_debugInfo_t elfFile_getDebugInfo(elfFile* me, void* address) {
    return reinterpret_cast<ElfFile*>(me->priv)->getDebugInfo(address);
}

void elfFile_destroy(binaryFile*) {}

void elfFile_delete(binaryFile* me) {
    auto self = elfFileOrNull(me);
    if (self == nullptr) {
        return;
    }

    delete reinterpret_cast<ElfFile*>(self->priv);
}
