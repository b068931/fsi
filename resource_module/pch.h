#ifndef PCH_H
#define PCH_H

#define LOGGER_MODULE_EMITTER_MODULE_NAME "RESOURCE MODULE (CORE)"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the resource module."
#endif

#include <Windows.h>
#include <iostream>
#include <cstdint>
#include <mutex>
#include <vector>
#include <map>
#include <iterator>
#include <new>
#include <stack>
#include <format>
#include <cassert>
#include <utility>
#include <format>
#include <algorithm>
#include <atomic>
#include <set>
#include <syncstream>

#endif
