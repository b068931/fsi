#ifndef FSI_TYPES_H
#define FSI_TYPES_H

#include <stdint.h>
#include "module_part.h"

namespace module_mediator {
	using one_byte = uint8_t;
	using signed_one_byte = int8_t;
	using two_bytes = uint16_t;
	using signed_two_bytes = int16_t;
	using four_bytes = uint32_t;
	using signed_four_bytes = int32_t;
	using eight_bytes = uint64_t;
	using signed_eight_bytes = int64_t;
	using pointer = void*;

	constexpr uint8_t one_byte_return_value = 0;
	constexpr uint8_t two_bytes_return_value = 1;
	constexpr uint8_t four_bytes_return_value = 2;
	constexpr uint8_t eight_bytes_return_value = 3;
	constexpr uint8_t pointer_return_value = 4;

	constexpr return_value execution_result_continue = 0;
	constexpr return_value execution_result_switch = 1;
	constexpr return_value execution_result_terminate = 2;
}

#endif