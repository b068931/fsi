#ifndef FSI_TYPES_H
#define FSI_TYPES_H

#include <cstdint>
#include "module_part.h"

namespace module_mediator {
	using one_byte = std::uint8_t;
	using signed_one_byte = std::int8_t;
	using two_bytes = std::uint16_t;
	using signed_two_bytes = std::int16_t;
	using four_bytes = std::uint32_t;
	using signed_four_bytes = std::int32_t;
	using eight_bytes = std::uint64_t;
	using signed_eight_bytes = std::int64_t;
	using memory = void*;

	inline constexpr std::uint8_t one_byte_return_value = 0;
	inline constexpr std::uint8_t two_bytes_return_value = 1;
	inline constexpr std::uint8_t four_bytes_return_value = 2;
	inline constexpr std::uint8_t eight_bytes_return_value = 3;
	inline constexpr std::uint8_t memory_return_value = 4;

	inline constexpr return_value execution_result_continue = 0;
	inline constexpr return_value execution_result_switch = 1;
	inline constexpr return_value execution_result_terminate = 2;
	inline constexpr return_value execution_result_block = 3;
}

#endif