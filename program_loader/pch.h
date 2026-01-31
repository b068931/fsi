#ifndef PCH_H
#define PCH_H

#define LOGGER_MODULE_EMITTER_MODULE_NAME "PROGRAM LOADER (CORE)"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the program loader module."
#endif

#include <Windows.h>
#include <map>
#include <memory>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <mutex>
#include <format>
#include <utility> //std::pair
#include <cstdint> //uintX_t, intX_t types
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <format>
#include <iostream>
#include <algorithm>
#include <ranges>
#include <bit>
#include <span>
#include <shared_mutex>

#endif

