#ifndef UNWIND_INFO_WIN32_H
#define UNWIND_INFO_WIN32_H

#include <winnt.h>
#include <cstdint>
#include <limits>

using UBYTE = unsigned char;
struct alignas(DWORD) UNWIND_INFO_HEADER {
    UBYTE Version : 3;
    UBYTE Flags : 5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister : 4;
    UBYTE FrameOffset : 4;
};

struct UNWIND_INFO_PROLOGUE {
    UNWIND_INFO_HEADER Header;
    USHORT UnwindCodes[1];
};

struct CHAINED_UNWIND_INFO {
    UNWIND_INFO_HEADER Header;
    RUNTIME_FUNCTION ChainedFunction;
};

constexpr std::size_t UNWIND_INFO_MAXIMUM_SIZE = sizeof(UNWIND_INFO_HEADER) + 
    std::numeric_limits<UBYTE>::max() * sizeof(USHORT);

static_assert(alignof(UNWIND_INFO_HEADER) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

static_assert(alignof(UNWIND_INFO_PROLOGUE) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

static_assert(alignof(CHAINED_UNWIND_INFO) == alignof(DWORD), 
    "UNWIND_INFO_HEADER must be DWORD-aligned.");

#endif
