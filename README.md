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

## Usage
In order to get started, you can either download a compiled version of the library [here][1].  
Alternatively, you can also build it from source:
- Clone the repository: ``git clone --recursive https://github.com/mhahnFr/CallstackLibrary.git``
- and build the library: ``cd CallstackLibrary && make``.

On some systems, you might need to install the ``libexecinfo-dev`` to compile the library successfully.
If this is the case, add ``-lexecinfo`` to the linking flags.

In order to use this library, simply include the header [``callstack.h``][2].

**Linking**:
- On **macOS**, link using these flags: ``-L<path/to/library> -lcallstack``.
- On **Linux**, link with ``-rdynamic -L<path/to/library> -lcallstack -ldl``.
- On **FreeBSD**, link with ``-rdynamic -L<path/to/library> -lcallstack -ldl -lexecinfo``.

The complete set of features is described in the [wiki][3].

### Final notes
This library is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2024 [mhahnFr][4]

[1]: https://github.com/mhahnFr/CallstackLibrary/releases
[2]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack.h
[3]: https://github.com/mhahnFr/CallstackLibrary/wiki
[4]: https://github.com/mhahnFr
[5]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack_exception.hpp
