# Welcome to the CallstackLibrary!
The CallstackLibrary is a library designed to create human-readable callstacks for natively compiled applications.

It can be used with pure **C**, although some optimizations and features using **C++** optionally can be enabled.

Its API is available for both **C** and **C++**.

## Quickstart
Use the CallstackLibrary for creating human-readable callstacks and for creating C++ exceptions that can print their
stacktrace.

1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git`
2. Build it: `cd CallstackLibrary && make`
3. Link your code with `-L<path/to/library> -lcallstack -ldl`

Now, you can use the headers [``callstack.h``][2] and [``callstack_exception.hpp``][5], respectively.

More explanation can be found in the [wiki][3]; the detailed explanation follows below.

## Usage
### Installation
Get started by either downloading a prebuilt version of this library [here][1].
Alternatively you can also build it from source:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git`
2. go into the cloned repository: `cd CallstackLibrary`
3. and build the library: `make`

> [!NOTE]
> On some systems, you might need to install the `libexecinfo-dev` to compile the library successfully.
> 
> If this is the case, add `-lexecinfo` to your linking flags: `-L<path/to/library> -lcallstack -ldl -lexecinfo`

Or in one step:
```shell
git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git && cd CallstackLibrary && make
```

To enable the optional optimizations using **C++** add `CXX_OPTIMIZED=true` as argument to `make`, for the optional
**C++** exclusive functions add `CXX_FUNCTIONS=true` as argument.

> [!TIP]
> **Example**:
> ```shell
> make CXX_OPTIMIZED=true CXX_FUNCTIONS=true
> ```

> [!NOTE]
> When statically linking against the CallstackLibrary with **C++** exclusive functions or optimizations enabled
> make sure to also link against the C++ standard library of your compiler (this is usually already the case when
> linking C++ code).

More information about the **C++** exclusive functions and optimizations [here][6].

Once you have a copy of the CallstackLibrary you can install it using the following command:
```shell
make INSTALL_PATH=/usr/local install
```
If you downloaded a [release][1] you can simply move the headers and the library anywhere you like.

#### Uninstallation
Uninstall the library by simply removing it and its header files from the installation directory.  
This can be done using the following command:
```shell
make INSTALL_PATH=/usr/local uninstall
```

### How to use
In order to use this library, simply include the header [``callstack.h``][2].

#### Linking
- Add `-L<path/to/library>` if the CallstackLibrary has not been installed in one of the default directories.
- On Linux and FreeBSD add `-rdynamic` to the linking flags.

Link with `-lcallstack`

> [!TIP]
> Examples:
> - **macOS**: `-L<path/to/library> -lcallstack`
> - **Linux**: `-rdynamic -L<path/to/library> -lcallstack -ldl`
> - **FreeBSD**: `-rdynamic -L<path/to/library> -lcallstack -ldl -lexecinfo`

### Callstacks
```C
// main.c

#include <stdio.h> // For printf(...)

#include <callstack.h>

void printCallstack(void) {
    struct callstack* callstack = callstack_new();
    struct callstack_frame* frames = callstack_toArray(callstack);

    printf("The current callstack:\n");
    for (size_t i = 0; i < callstack_getFrameCount(callstack); ++i) {
        printf("In: (%s) %s (%s:%ld)\n", callstack_frame_getShortestName(&frames[i]), 
                                         frames[i].function,
                                         frames[i].sourceFile == NULL ? "???" : callstack_frame_getShortestSourceFile(&frames[i]),
                                         frames[i].sourceLine);
    }
    callstack_delete(callstack);
}

void bar(void) { printCallstack(); }

void foo(void)  { bar();  }
void bar2(void) { foo();  }
void foo2(void) { bar2(); }

int main(void) {
    foo2();
}
```
Compiled and linked on macOS with `cc -g main.c -I<path/to/library>/include -L<path/to/library> -lcallstack` creates the following output:
```
The current callstack:
In: (a.out) printCallstack (main.c:8)
In: (a.out) bar (main.c:21)
In: (a.out) foo (main.c:23)
In: (a.out) bar2 (main.c:24)
In: (a.out) foo2 (main.c:25)
In: (a.out) main (main.c:28)
In: (/usr/lib/dyld) start + 1903 (???:0)
```

#### C++
The [example above][7] can be written in C++ using the C++ wrapper class as follows:
```C++
// main.cpp

#include <iostream>

#include <callstack.h>

void printCallstack() {
    lcs::callstack callstack;
    callstack_frame* frames = callstack_toArray(callstack);
    
    std::cout << "The current callstack:" << std::endl;
    for (size_t i = 0; i < callstack_getFrameCount(callstack); ++i) {
        std::cout << "In: (" << callstack_frame_getShortestName(&frames[i])
                  << ") "    << frames[i].function
                  << " ("    << (frames[i].sourceFile == NULL ? "???" : callstack_frame_getShortestSourceFile(&frames[i]))
                  << ":"     << frames[i].sourceLine
                  << ")"     << std::endl;
    }   
}

void bar() { printCallstack(); }

void foo()  { bar();  }
void bar2() { foo();  }
void foo2() { bar2(); }

int main() {
    foo2();
}
```
Compiled and linked on macOS with `c++ -g main.cpp -I<path/to/library>/include -L<path/to/library> -lcallstack` and
after [enabling **C++** functions][6] of the library:
```
The current callstack:
In: (a.out) lcs::callstack::callstack(bool) (include/callstack.hpp:77)
In: (a.out) printCallstack() (main.cpp:8)
In: (a.out) bar() (main.cpp:21)
In: (a.out) foo() (main.cpp:23)
In: (a.out) bar2() (main.cpp:24)
In: (a.out) foo2() (main.cpp:25)
In: (a.out) main (main.cpp:28)
In: (/usr/lib/dyld) start + 1903 (???:0)
```

> [!TIP]
> The **C++** functions can be enabled as described [here][6].

### Callstack exceptions
With the [callstack exception][8] an exception capable of printing its construction stacktrace is available.

It can be thrown directly:
```C++
// main.cpp

#include <iostream>

#include <callstack_exception.hpp>

void printCallstack() {
    throw lcs::exception("Callstack exception with a message");
}

void bar2() { printCallstack(); }
void foo2() { bar2();           }

void bar() { foo2(); }
void foo() { bar();  }

int main() {
    try {
        foo();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
```
Compiled and linked on macOS using `c++ -g main.cpp -I<path/to/library>/include -L<path/to/library> -lcallstack` and
after [enabling **C++** functions][6] of the library creates the following output:
```
lcs::exception: "Callstack exception with a message", stacktrace:
At: (a.out) lcs::exception::exception(char const*, bool) (include/callstack_exception.hpp:123)
in: (a.out) printCallstack() (main.cpp:8)
in: (a.out) bar2() (main.cpp:11)
in: (a.out) foo2() (main.cpp:12)
in: (a.out) bar() (main.cpp:14)
in: (a.out) foo() (main.cpp:15)
in: (a.out) main (main.cpp:19)
in: (/usr/lib/dyld) start + 1903
```

#### Extending the callstack exception
The [callstack exception][8] can easily serve as base class for other exceptions:
```C++
// main.cpp

#include <iostream>

#include <callstack_exception.hpp>

class CustomStacktraceException: public lcs::exception {};

void printCallstack() {
    throw CustomStacktraceException();
}

void bar2() { printCallstack(); }
void foo2() { bar2();           }

void bar() { foo2(); }
void foo() { bar();  }

int main() {
    try {
        foo();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
```
Compiled and linked on macOS using `c++ -g main.cpp -I<path/to/library>/include -L<path/to/library> -lcallstack` and
after [enabling **C++** functions][6] of the library creates the following output:
```
CustomStacktraceException, stacktrace:
At: (a.out) CustomStacktraceException::CustomStacktraceException() (main.cpp:7)
in: (a.out) CustomStacktraceException::CustomStacktraceException() (main.cpp:7)
in: (a.out) printCallstack() (main.cpp:10)
in: (a.out) bar2() (main.cpp:13)
in: (a.out) foo2() (main.cpp:14)
in: (a.out) bar() (main.cpp:16)
in: (a.out) foo() (main.cpp:17)
in: (a.out) main (main.cpp:21)
in: (/usr/lib/dyld) start + 1903
```

## Symbolization
The generated callstacks are generally symbolized using the information obtained by the dynamic loader (hence the
dependency of the `libdl`).

### macOS
On macOS the debug information available in the Mach-O binaries is used. The following kinds of debug symbols are supported:
- `.dSYM` bundles
- `Mach-O` debug symbol maps (DWARF inside the object files)

> [!NOTE]
> The parser currently supports DWARF in version 4.

> [!TIP]
> Usually the appropriate compilation flag for debug symbols is `-g`.

### Linux and FreeBSD
Currently only the information obtained by the dynamic loader is available on Linux and FreeBSD.

> [!TIP]
> For the best results, link your code with the flag `-rdynamic`.

## Final notes
This library is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2024 [mhahnFr][4]

[1]: https://github.com/mhahnFr/CallstackLibrary/releases/latest
[2]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack.h
[3]: https://github.com/mhahnFr/CallstackLibrary/wiki
[4]: https://github.com/mhahnFr
[5]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack_exception.hpp
[6]: https://github.com/mhahnFr/CallstackLibrary/wiki/Home#enabling-additional-c-exclusive-functions
[7]: #callstacks
[8]: https://github.com/mhahnFr/CallstackLibrary/wiki/callstack.hpp#class-callstack
