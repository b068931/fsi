#ifndef PCH_H
#define PCH_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#include <windows.h>
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

#endif