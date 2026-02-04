#ifndef UNWIND_INFO_WIN32_H
#define UNWIND_INFO_WIN32_H

#include <winnt.h>
#include <cstdint>
#include <limits>

// This is to satisfy more formal C++ requirements for object creation.
// We are going to use std::memcpy for implicit object creation, so ensure that
// compiler knows what object size we are dealing with.
constexpr std::size_t EXPECTED_UNWIND_CODES_COUNT = 10;

using UBYTE = unsigned char;
struct alignas(DWORD) UNWIND_INFO_HEADER {
    UBYTE Version : 3;
    UBYTE Flags : 5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister : 4;
    UBYTE FrameOffset : 4;
};

struct UNWIND_INFO_DISPATCHER_PROLOGUE {
    UNWIND_INFO_HEADER Header;
    USHORT UnwindCodes[EXPECTED_UNWIND_CODES_COUNT];
};

struct CHAINED_UNWIND_INFO {
    UNWIND_INFO_HEADER Header;
    RUNTIME_FUNCTION ChainedFunction;
};

constexpr std::size_t DISPATCHER_UNWIND_INFO_SIZE = 
    sizeof(UNWIND_INFO_DISPATCHER_PROLOGUE);

static_assert(alignof(UNWIND_INFO_HEADER) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

static_assert(alignof(UNWIND_INFO_DISPATCHER_PROLOGUE) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

static_assert(alignof(CHAINED_UNWIND_INFO) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

#endif
