#ifndef UNWIND_INFO_WIN32_H
#define UNWIND_INFO_WIN32_H

using UBYTE = unsigned char;
struct UNWIND_INFO_HEADER {
    UBYTE Version : 3;
    UBYTE Flags : 5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister : 4;
    UBYTE FrameOffset : 4;
};

struct CHAINED_UNWIND_INFO {
    UNWIND_INFO_HEADER Header;
    RUNTIME_FUNCTION ChainedFunction;
};

#endif
