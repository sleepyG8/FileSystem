# Makefile for compiling Vdisk.cpp

# Compiler and flags for GCC
GCC = gcc
GCC_FLAGS = -Wall -o Vdisk

# Compiler and flags for CL (MSVC)
CL = cl
CL_FLAGS = /Fe:Vdisk.exe

# Source file
SRC = Vdisk.cpp

# Default target
all:
ifeq ($(OS),Windows_NT)
	$(CL) $(SRC) $(CL_FLAGS)
else
	$(GCC) $(SRC) $(GCC_FLAGS)
endif

# Clean target to remove executables
clean:
ifeq ($(OS),Windows_NT)
	del Vdisk.exe
else
	rm -f Vdisk
endif
