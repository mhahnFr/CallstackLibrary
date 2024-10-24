# Welcome to the CallstackLibrary!
The CallstackLibrary is a library designed to create human-readable callstacks for natively compiled applications.

It can be used with pure **C**, although some features using **C++** optionally can be enabled.

Its API is available for both **C** and **C++**.

This stacktrace library is designed to be fast and to have almost no dependencies, with the notable exception being the
usually preinstalled `libexecinfo` (for the function `backtrace`).

## Quickstart
Use the CallstackLibrary for creating human-readable callstacks and for creating C++ exceptions that can print their
stacktrace.

Either [download a release here][1] and use the prebuilt library and the headers.

Alternatively easily build it yourself:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git`
2. Build it: `cd CallstackLibrary && make -j`
3. Link your code with `-L<path/to/library> -lcallstack`

Now, you can use the headers [`callstack.h`][2] and [`callstack_exception.hpp`][5], respectively.

More explanation can be found in the [wiki][3]; the detailed explanation follows below.

## Usage
### Installation
Get started by either downloading a prebuilt version of this library [here][1].

Alternatively you can also build it from source:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git`
2. go into the cloned repository: `cd CallstackLibrary`
3. and build the library: `make -j`

Or in one step:
```shell
git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git && cd CallstackLibrary && make -j
```

To enable the optional **C++** exclusive functions add `CXX_FUNCTIONS=true` as argument to `make`.

> [!TIP]
> **Example**:
> ```shell
> make -j CXX_FUNCTIONS=true
> ```

> [!NOTE]
> When statically linking against the CallstackLibrary with **C++** exclusive functions enabled make sure to also link
> against the C++ standard library of your compiler (this is usually already the case when linking C++ code).

More information about the **C++** exclusive functions [here][6].

If you want to install the library, which is *not* necessary for it to work properly, you can do so using the
following command:
```shell
make INSTALL_PATH=/usr/local install
```
Adapt the value of the `INSTALL_PATH` argument to your needs.

If you downloaded a [release][1] you can simply move the headers and the library anywhere you like.

#### Build dependencies
The following dependencies are needed in order to successfully build the library:
- C11 compatible compiler with GNU language extensions
- Standard C library with the following additional functions:
    - `strdup` (POSIX.1-2001)
    - `asprintf` (POSIX.1-2024)
    - `backtrace`

##### Linux
The following additional Linux specific dependencies are needed:
- GNU standard C library

##### macOS
Currently, no additional macOS specific dependencies are necessary.

### Uninstallation
Uninstall the library by simply removing it and its header files from the installation directory.  
This can be done using the following command:
```shell
make INSTALL_PATH=/usr/local uninstall
```
Adapt the value of the `INSTALL_PATH` argument to your needs.

### How to use
In order to use this library, simply include the header [`callstack.h`][2] and [`callstack_exception.hpp`][5].

#### Linking
> [!NOTE]
> Add `-L<path/to/library>` if the CallstackLibrary has not been installed in one of the default directories.

Link with `-lcallstack`

> [!TIP]
> Example: `-L<path/to/library> -lcallstack`

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
                                         (frames[i].function == NULL ? "???" : frames[i].function),
                                         callstack_frame_getShortestSourceFileOr(&frames[i], "???"),
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
Compiled and linked on macOS with `cc -g main.c -I<path/to/library>/include -L<path/to/library> -lcallstack` the example
creates the following output:
```
The current callstack:
In: (a.out) printCallstack (main.c:8)
In: (a.out) bar (main.c:21)
In: (a.out) foo (main.c:23)
In: (a.out) bar2 (main.c:24)
In: (a.out) foo2 (main.c:25)
In: (a.out) main (main.c:28)
In: (/usr/lib/dyld) start + 1942 (???:0)
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
                  << ") "    << (frames[i].function == NULL ? "???" : frames[i].function)
                  << " ("    << callstack_frame_getShortestSourceFileOr(&frames[i], "???")
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
Compiled and linked on Debian with `g++ -g main.cpp -I<path/to/library>/include -L<path/to/library> -lcallstack` and
after [enabling **C++** functions][6] of the library the following output is produced:
```
The current callstack:
In: (a.out) lcs::callstack::callstack(bool) (include/callstack.hpp:81)
In: (a.out) printCallstack() (main.cpp:8)
In: (a.out) bar() (main.cpp:21)
In: (a.out) foo() (main.cpp:23)
In: (a.out) bar2() (main.cpp:24)
In: (a.out) foo2() (main.cpp:25)
In: (a.out) main (main.cpp:28)
In: (/usr/lib/x86_64-linux-gnu/libc.so.6) ??? (???:0)
In: (/usr/lib/x86_64-linux-gnu/libc.so.6) __libc_start_main + 133 (???:0)
In: (a.out) _start + 33 (???:0)
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
after [enabling **C++** functions][6] of the library the following output is produced:
```
lcs::exception: "Callstack exception with a message", stacktrace:
At: (a.out) lcs::callstack::callstack(bool) (include/callstack.hpp:81)
in: (a.out) lcs::callstack::callstack(bool) (include/callstack.hpp:79)
in: (a.out) lcs::exception::exception(char const*, bool) (include/callstack_exception.hpp:126)
in: (a.out) lcs::exception::exception(char const*, bool) (include/callstack_exception.hpp:127)
in: (a.out) printCallstack() (main.cpp:8)
in: (a.out) bar2() (main.cpp:11)
in: (a.out) foo2() (main.cpp:12)
in: (a.out) bar() (main.cpp:14)
in: (a.out) foo() (main.cpp:15)
in: (a.out) main (main.cpp:19)
in: (/usr/lib/dyld) start + 1942
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
Compiled and linked on Debian using `g++ -g main.cpp -I<path/to/library>/include -L<path/to/library> -lcallstack` and
after [enabling **C++** functions][6] of the library the example creates the following output:
```
CustomStacktraceException, stacktrace:
At: (a.out) lcs::callstack::callstack(bool) (include/callstack.hpp:81)
in: (a.out) lcs::exception::exception(bool) (include/callstack_exception.hpp:116)
in: (a.out) CustomStacktraceException::CustomStacktraceException() (main.cpp:7)
in: (a.out) printCallstack() (main.cpp:10)
in: (a.out) bar2() (main.cpp:13)
in: (a.out) foo2() (main.cpp:14)
in: (a.out) bar() (main.cpp:16)
in: (a.out) foo() (main.cpp:17)
in: (a.out) main (main.cpp:21)
in: (/usr/lib/x86_64-linux-gnu/libc.so.6) << Unknown >>
in: (/usr/lib/x86_64-linux-gnu/libc.so.6) __libc_start_main + 133
in: (a.out) _start + 33
```

## Symbolization
The callstacks are generated using the function `backtrace` of the `libexecinfo`, which is commonly preinstalled.

> [!NOTE]
> If this is not the case, you probably need to add `-lexecinfo` to the linking flags of the library.

The generated callstacks are symbolized using an **ELF** file parser on Linux and a **Mach-O** file parser on macOS.  
They are enriched using the appropriate **DWARF** debugging information that is available.

The DWARF parser supports **DWARF** in version **2**, **3**, **4** and **5**.

> [!TIP]
> Usually the appropriate compilation flag for debug symbols is `-g`.

### macOS
On macOS the debug information available in the **Mach-O** binaries is used. The following kinds of debug symbols are
supported:
- `.dSYM` bundles
- **Mach-O** debug symbol maps (**DWARF** inside the object files)

### Linux
On Linux the debug information available in the **ELF** binaries is used.

## Final notes
If you experience any problems with the CallstackLibrary or if you have ideas to further improve this library don't
hesitate to [open an issue][9] or a [pull request][10].

This library is licensed under the terms of the GNU GPL in version 3 or later.

© Copyright 2022 - 2024 [mhahnFr][4]

[1]: https://github.com/mhahnFr/CallstackLibrary/releases/latest
[2]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack.h
[3]: https://github.com/mhahnFr/CallstackLibrary/wiki
[4]: https://github.com/mhahnFr
[5]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack_exception.hpp
[6]: https://github.com/mhahnFr/CallstackLibrary/wiki/Home#enabling-additional-c-exclusive-functions
[7]: #callstacks
[8]: https://github.com/mhahnFr/CallstackLibrary/wiki/callstack.hpp#class-callstack
[9]: https://github.com/mhahnFr/CallstackLibrary/issues/new
[10]: https://github.com/mhahnFr/CallstackLibrary/pulls