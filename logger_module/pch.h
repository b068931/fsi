#ifndef PCH_H
#define PCH_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the logger module."
#endif

#include <windows.h>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <format>
#include <chrono>

#endif
