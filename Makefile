#
# Callstack Library - Library creating human-readable call stacks.
#
# Copyright (C) 2022 - 2023  mhahnFr
#
# This file is part of the CallstackLibrary. This library is free software:
# you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
#

CXX_FUNCTIONS = false
CXX_OPTIMIZED = false

# Library names
CORE_NAME = libcallstack
DYLIB_N   = $(CORE_NAME).dylib
SHARED_N  = $(CORE_NAME).so
STATIC_N  = $(CORE_NAME).a
# -------------

LD = $(CC)

# Paths
OPTIMIZED_PATH = cpp_optimized/
LINUX_PATH     = ./src/parser/file/elf
DARWIN_PATH    = ./src/parser/file/macho
# -----

# Assert submodules are available
ifeq ($(shell ls DC4C),)
	_  = $(shell git submodule init)
	_ += $(shell git submodule update)
endif
# -------------------------------

# Main sources
SRCS = $(shell find ./src -type f -name \*.c \! -path $(LINUX_PATH)\* \! -path $(DARWIN_PATH)\* \! -path \*/$(OPTIMIZED_PATH)\*)
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.d, $(SRCS))
# ------------

# C++ sources
CXX_SRCS = $(shell find . -type f -name \*.cpp \! -path \*/$(OPTIMIZED_PATH)\*)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_SRCS))
CXX_DEPS = $(patsubst %.cpp, %.d, $(CXX_SRCS))
# -----------

# Linux specific sources
LINUX_SRCS = $(shell find $(LINUX_PATH) -type f -name \*.c \! -path \*/$(OPTIMIZED_PATH)\*)
LINUX_OBJS = $(patsubst %.c, %.o, $(LINUX_SRCS))
LINUX_DEPS = $(patsubst %.c, %.d, $(LINUX_SRCS))
ifeq ($(CXX_OPTIMIZED),true)
	LD = $(CXX)
	LINUX_OPT = $(shell find $(LINUX_PATH) -type f -name \*.cpp -path \*/$(OPTIMIZED_PATH)\*)
	
	LINUX_OBJS += $(patsubst %.cpp, %.o, $(LINUX_OPT))
	LINUX_DEPS += $(patsubst %.cpp, %.d, $(LINUX_OPT))
else
	LINUX_OPT = $(shell find $(LINUX_PATH) -type f -name \*.c -path \*/$(OPTIMIZED_PATH)\*)
	
	LINUX_OBJS += $(patsubst %.c, %.o, $(LINUX_OPT))
	LINUX_DEPS += $(patsubst %.c, %.d, $(LINUX_OPT))
endif
# ----------------------

# Darwin specific sources
DARWIN_SRCS = $(shell find $(DARWIN_PATH) -type f -name \*.c \! -path \*/$(OPTIMIZED_PATH)\*)
DARWIN_OBJS = $(patsubst %.c, %.o, $(DARWIN_SRCS))
DARWIN_DEPS = $(patsubst %.c, %.d, $(DARWIN_SRCS))
ifeq ($(CXX_OPTIMIZED),true)
	LD = $(CXX)
	DARWIN_OPT = $(shell find $(DARWIN_PATH) -type f -name \*.cpp -path \*/$(OPTIMIZED_PATH)\*)

	DARWIN_OBJS += $(patsubst %.cpp, %.o, $(DARWIN_OPT))
	DARWIN_DEPS += $(patsubst %.cpp, %.d, $(DARWIN_OPT))
else
	DARWIN_OPT = $(shell find $(DARWIN_PATH) -type f -name \*.c -path \*/$(OPTIMIZED_PATH)\*)

	DARWIN_OBJS += $(patsubst %.c, %.o, $(DARWIN_OPT))
	DARWIN_DEPS += $(patsubst %.c, %.d, $(DARWIN_OPT))
endif
# -----------------------

# Compile and link flags
COM_FLAGS = -Wall -Wextra -fPIC -Ofast
CFLAGS    = $(COM_FLAGS) -std=gnu11
CXXFLAGS  = $(COM_FLAGS) -std=gnu++17
LDFLAGS   = -ldl
# ----------------------

NAME = $(STATIC_N)

ifeq ($(CXX_FUNCTIONS),true)
	LD      = $(CXX)
	OBJS   += $(CXX_OBJS)
	CFLAGS += -DCXX_FUNCTIONS
	DEPS   += $(CXX_DEPS)
endif

ifeq ($(shell uname -s),Darwin)
	LDFLAGS += -current_version 1.0.0 -compatibility_version 1 -install_name $(abspath $@)
	OBJS    += $(DARWIN_OBJS)
	DEPS    += $(DARWIN_DEPS)
else ifeq ($(shell uname -s),Linux)
	OBJS += $(LINUX_OBJS)
	DEPS += $(LINUX_DEPS)
else
$(error Unsupported platform)
endif

INSTALL_PATH ?= /usr/local

default: $(NAME)

all: $(SHARED_N) $(STATIC_N) $(DYLIB_N)

install: $(SHARED_N)
	mkdir -p $(INSTALL_PATH)/lib
	mkdir -p "$(INSTALL_PATH)/include"
	cp $(SHARED_N) $(INSTALL_PATH)/lib
	find "include" \( -name \*.h -o -name \*.hpp \) -exec cp {} "$(INSTALL_PATH)/include" \;

uninstall:
	- $(RM) $(INSTALL_PATH)/lib/$(SHARED_N)
	- $(RM) $(addprefix $(INSTALL_PATH)/, $(shell find "include" -name \*.h -o -name \*.hpp))

$(DYLIB_N): $(OBJS)
	$(LD) -dynamiclib $(LDFLAGS) -o $(DYLIB_N) $(OBJS)

$(SHARED_N): $(OBJS)
	$(LD) -shared -fPIC $(LDFLAGS) -o $(SHARED_N) $(OBJS)

$(STATIC_N): $(OBJS)
	$(AR) -crs $(STATIC_N) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	- $(RM) $(OBJS) $(DEPS)
	- $(RM) $(CXX_OBJS) $(CXX_DEPS)

fclean: clean
	- $(RM) $(DYLIB_N) $(SHARED_N) $(STATIC_N)

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all default install uninstall

-include $(DEPS)
