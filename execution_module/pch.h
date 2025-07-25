#ifndef PCH_H
#define PCH_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef NDEBUG
#include <vld.h>
#endif

#include <windows.h>
#include <cassert>
#include <new>
#include <list>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <atomic>
#include <format>
#include <vector>
#include <thread>
#include <format>
#include <map>
#include <ranges>

#endif //PCH_H