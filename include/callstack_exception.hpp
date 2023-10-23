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

#ifndef callstack_exception_hpp
#define callstack_exception_hpp

#include <exception>
#include <ostream>
#include <sstream>
#include <string>

#include "callstack.h"

#if __cplusplus >= 201103
 #define LCS_NOEXCEPT  noexcept
 #define LCS_CONSTEXPR constexpr
 #define LCS_OVERRIDE  override
 #define LCS_NULL      nullptr

 #define LCS_CXX11

#else
 #define LCS_NOEXCEPT  throw()
 #define LCS_CONSTEXPR
 #define LCS_OVERRIDE
 #define LCS_NULL      NULL
#endif

namespace lcs {
class exception: public std::exception {
    const std::string message;
    
    bool shouldPrintStacktrace;
    
    mutable callstack cs;
    mutable std::string messageBuffer;
    
    static inline std::string toString(unsigned long number) {
#ifdef LCS_CXX11
        return std::to_string(number);
#else
        std::stringstream stream;
        stream << number;
        return stream.str();
#endif
    }
    
public:
    exception(const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    exception(const char * message, const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(message), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    exception(const std::string & message, const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(message), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    exception(const exception & other)
        : std::exception(other), message(other.message), shouldPrintStacktrace(other.shouldPrintStacktrace), cs(other.cs) {}
    
   ~exception() LCS_NOEXCEPT {};
    
#ifdef LCS_CXX11
    exception(exception &&) = default;
#endif
    
    virtual const char * what() const LCS_NOEXCEPT LCS_OVERRIDE {
        if (!messageBuffer.empty()) {
            return messageBuffer.c_str();
        }
        
        if (shouldPrintStacktrace) {
            std::stringstream stream;
            printStacktrace(stream, true);
            messageBuffer = stream.str();
        } else {
            messageBuffer = "lcs::exception" + (message.empty() ? "" : (": \"" + message + "\""));
        }
        return messageBuffer.c_str();
    }
    
    void printStacktrace(std::ostream & out, const bool printMessage = true) const {
        if (printMessage) {
            out << "lcs::exception" << (message.empty() ? "" : ": \"" + message + "\"") << std::endl;
        }
        const callstack_frame * frames = callstack_toArray(cs);
        for (std::size_t i = 0; i < callstack_getFrameCount(cs); ++i) {
            out << (i == 0 ? "At" : "in") << ": "
                << frames[i].function << " ("
                << (frames[i].sourceFile == LCS_NULL
                    ? frames[i].binaryFile
                    : (std::string(frames[i].sourceFile) + ":" + toString(frames[i].sourceLine)))
                << ")" << std::endl;
        }
    }
    
    void setPrintStacktrace(const bool printStacktrace) LCS_NOEXCEPT {
        shouldPrintStacktrace = printStacktrace;
    }
    
    LCS_CONSTEXPR bool getPrintStacktrace() const LCS_NOEXCEPT {
        return shouldPrintStacktrace;
    }
    
    LCS_CONSTEXPR callstack & getCallstack() const LCS_NOEXCEPT {
        return cs;
    }
    
    LCS_CONSTEXPR const std::string & getMessage() const LCS_NOEXCEPT {
        return message;
    }
};
}

#endif /* callstack_exception_hpp */
