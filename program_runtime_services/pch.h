#ifndef PCH_H
#define PCH_H

#define LOGGER_MODULE_EMITTER_MODULE_NAME "PROGRAM RUNTIME SERVICES (CORE)"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the program runtime services module."
#endif

#include <Windows.h>
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
