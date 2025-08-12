#ifndef STRUCTURED_EXCEPTION_HANDLING_SETUP_H
#define STRUCTURED_EXCEPTION_HANDLING_SETUP_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL InstallGlobalCrashHandler();

#endif