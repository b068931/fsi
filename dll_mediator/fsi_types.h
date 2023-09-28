#ifndef FSI_TYPES_H
#define FSI_TYPES_H

#include <stdint.h>
#include "dll_part.h"

using byte = uint8_t;
using signed_byte = int8_t;
using dbyte = uint16_t;
using signed_dbyte = int16_t;
using fbyte = uint32_t;
using signed_fbyte = int32_t;
using ebyte = uint64_t;
using signed_ebyte = int64_t;
using pointer = void*;

constexpr uint8_t byte_return_value = 0;
constexpr uint8_t dbyte_return_value = 1;
constexpr uint8_t fbyte_return_value = 2;
constexpr uint8_t ebyte_return_value = 3;
constexpr uint8_t pointer_return_value = 4;

constexpr return_value execution_result_continue = 0;
constexpr return_value execution_result_switch = 1;
constexpr return_value execution_result_terminate = 2;

#endif