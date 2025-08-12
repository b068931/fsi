#ifndef PCH_H
#define PCH_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the program runtime services module."
#endif

#include <windows.h>
#include <memory>
#include <vector>
#include <utility>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <format>
#include <shared_mutex>
#include <vector>
#include <ranges>
#include <thread>
#include <limits>
#include <barrier>
#include <filesystem>
#include <cassert>
#include <atomic>

#endif //PCH_H