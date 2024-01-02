# Welcome to the CallstackLibrary!
This repository contains a library which can create human-readable callstacks. It features a C API, but
there is also a C++ wrapper for it.

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

## Upcoming features
The callstacks are currently created using the information of the dynamic linker.
In the future, parsing of the debug symbols will be added.

### Final notes
This library is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2024 [mhahnFr][4]

[1]: https://github.com/mhahnFr/CallstackLibrary/releases
[2]: https://github.com/mhahnFr/CallstackLibrary/blob/main/include/callstack.h
[3]: https://github.com/mhahnFr/CallstackLibrary/wiki
[4]: https://github.com/mhahnFr
