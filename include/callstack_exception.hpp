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

#include <cxxabi.h>
#include <exception>
#include <ostream>
#include <sstream>
#include <string>
#include <typeinfo>

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

/**
 * @brief The namespace all C++ classes of the CallstackLibrary can be found in.
 *
 * The name stands for: **l**ib**c**all**s**tack.
 */
namespace lcs {
/**
 * This exception class defines an exception capable to create and print
 * the callstack where it initially has been constructed - usually where it was thrown.
 */
class exception: public std::exception {
    /** The optional message of this exception.                                  */
    const std::string message;
    
    /** Whether to automatically translate and add the callstack to the message. */
    bool shouldPrintStacktrace;
    
    /** The callstack where this object was constructed.                         */
    mutable callstack cs;
    /** A message buffer.                                                        */
    mutable std::string messageBuffer;
    
    /**
     * Converts the given number to a string and returns it.
     *
     * @param number the number to be converted
     * @return the string representation of the given number
     */
    static inline std::string toString(unsigned long number) {
#ifdef LCS_CXX11
        return std::to_string(number);
#else
        std::stringstream stream;
        stream << number;
        return stream.str();
#endif
    }
    
    /**
     * Returns the demangled name of the class the calling object belongs to.
     *
     * @return the name of the class of the calling object
     */
    inline std::string getName() const LCS_NOEXCEPT {
        std::string toReturn;
        
        const char * rawName = typeid(*this).name();
        int status;
        char * dName = abi::__cxa_demangle(rawName, LCS_NULL, LCS_NULL, &status);
        if (dName != LCS_NULL) {
            toReturn = dName;
            std::free(dName);
        } else {
            toReturn = rawName;
        }
        return toReturn;
    }
    
public:
    /**
     * Trivial default constructor.
     *
     * @param printStacktrace whether to automatically append the stacktrace to the exception message
     */
    explicit inline exception(const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    /**
     * @brief Constructs an exception with the given message.
     *
     * The message is copied.
     *
     * @param message the message
     * @param printStacktrace whether to automatically append the stacktrace
     */
    explicit inline exception(const char * message, const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(message), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    /**
     * Constructs an exception with the given message.
     *
     * @param message the message
     * @param printStacktrace whether to automatically append the stacktrace
     */
    explicit inline exception(const std::string & message, const bool printStacktrace = true) LCS_NOEXCEPT
        : std::exception(), message(message), shouldPrintStacktrace(printStacktrace), cs(__builtin_return_address(0)) {}
    
    inline exception(const exception & other)
        : std::exception(other), message(other.message), shouldPrintStacktrace(other.shouldPrintStacktrace), cs(other.cs) {}
    
    inline virtual ~exception() LCS_NOEXCEPT {};
    
#ifdef LCS_CXX11
    inline exception(exception &&) = default;
#endif
    
    inline virtual const char * what() const LCS_NOEXCEPT LCS_OVERRIDE {
        if (!messageBuffer.empty()) {
            return messageBuffer.c_str();
        }
        
        if (shouldPrintStacktrace) {
            std::stringstream stream;
            printStacktrace(stream, true);
            messageBuffer = stream.str();
        } else {
            messageBuffer = getName() + (message.empty() ? "" : (": \"" + message + "\""));
        }
        return messageBuffer.c_str();
    }
    
    /**
     * Prints the stacktrace where this exception has been constructed to the given output
     * stream.
     *
     * @param out the output stream to print to
     * @param printMessage whether to print the message text as well
     */
    inline void printStacktrace(std::ostream & out, const bool printMessage = true) const {
        if (printMessage) {
            out << getName() << (message.empty() ? "" : ": \"" + message + "\"");
        }
        out << ", stacktrace:" << std::endl;
        const callstack_frame * frames = callstack_toArray(cs);
        for (std::size_t i = 0; i < callstack_getFrameCount(cs); ++i) {
            out << (i == 0 ? "At" : "in") << ": "
                << "(" << callstack_frame_getShortestName(&frames[i]) << ") "
                << (frames[i].function == LCS_NULL ? "<< Unknown >>" : frames[i].function)
                << (frames[i].sourceFile == LCS_NULL ? ""
                    : (" (" + std::string(frames[i].sourceFile) + ":" + toString(frames[i].sourceLine) + ")"))
                << std::endl;
        }
    }
    
    /**
     * Sets whether to automatically append the stacktrace.
     *
     * @param printStacktrace whether to automatically append the stacktrace
     */
    inline void setPrintStacktrace(const bool printStacktrace) LCS_NOEXCEPT {
        shouldPrintStacktrace = printStacktrace;
    }
    
    /**
     * Returns whether to automatically append the stacktrace.
     *
     * @return whether to automatically append the stacktrace
     */
    inline LCS_CONSTEXPR bool getPrintStacktrace() const LCS_NOEXCEPT {
        return shouldPrintStacktrace;
    }
    
    /**
     * Returns the callstack where this exception has been constructed.
     *
     * @return the construction callstack
     */
    inline LCS_CONSTEXPR callstack & getCallstack() const LCS_NOEXCEPT {
        return cs;
    }
    
    /**
     * Returns the message of this exception.
     *
     * @return the message text
     */
    inline LCS_CONSTEXPR const std::string & getMessage() const LCS_NOEXCEPT {
        return message;
    }
};
}

#endif /* callstack_exception_hpp */
