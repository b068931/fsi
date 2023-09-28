#ifndef JUMP_TABLE_BUILDER_H
#define JUMP_TABLE_BUILDER_H

#include "variable_with_id.h" //entity_id

#include <stdint.h> //uint32_t, uint64_t
#include <vector>
#include <utility> //std::pair
#include <tuple>

class jump_table_builder {
private:
	std::vector<std::pair<entity_id, uint64_t>> function_addresses;
	std::vector<std::tuple<entity_id, uint32_t, uint32_t, uint64_t>> jump_addresses;

	static void copy_uint64_t(char* destination, uint64_t value_to_write) {
		std::memcpy(destination, reinterpret_cast<const char*>(&value_to_write), sizeof(uint64_t));
	}
public:
	void add_new_function_address(entity_id id, uint64_t address = {}) {
		this->function_addresses.push_back({ id, address });
	}
	void add_new_jump_address(entity_id id, uint32_t function_index, uint32_t instruction_index, uint64_t address = {}) {
		this->jump_addresses.push_back({ id, function_index, instruction_index, address });
	}

	void remap_function_address(entity_id id, uint64_t new_address) {
		auto found_function = std::find_if(this->function_addresses.begin(), this->function_addresses.end(),
			[id](const std::pair<entity_id, uint64_t>& value) {
				return value.first == id;
			}
		);
		if (found_function != this->function_addresses.end()) {
			found_function->second = new_address;
		}
	}

	void remap_jump_address(uint32_t function_index, uint32_t instruction_index, uint64_t new_address) {
		auto found_jump_point = std::find_if(this->jump_addresses.begin(), this->jump_addresses.end(),
			[function_index, instruction_index](const std::tuple<entity_id, uint32_t, uint32_t, uint64_t>& value) {
				return (std::get<1>(value) == function_index) && (std::get<2>(value) == instruction_index);
			}
		);
		if (found_jump_point != this->jump_addresses.end()) {
			std::get<3>(*found_jump_point) = new_address;
		}
	}
	void add_jump_base_address(uint32_t function_index, uint64_t address) {
		for (auto& jump_address : this->jump_addresses) {
			if (std::get<1>(jump_address) == function_index) {
				std::get<3>(jump_address) += address;
			}
		}
	}

	size_t get_function_table_index(entity_id id) {
		auto found_function = std::find_if(this->function_addresses.begin(), this->function_addresses.end(),
			[id](const std::pair<entity_id, uint64_t>& value) {
				return value.first == id;
			}
		);
		if (found_function != this->function_addresses.end()) {
			return sizeof(uint64_t) + sizeof(uint64_t) * (found_function - this->function_addresses.begin());
		}

		return 0;
	}
	size_t get_jump_point_table_index(entity_id id) {
		auto found_jump_point = std::find_if(this->jump_addresses.begin(), this->jump_addresses.end(),
			[id](const std::tuple<entity_id, uint32_t, uint32_t, uint64_t>& value) {
				return std::get<0>(value) == id;
			}
		);
		if (found_jump_point != this->jump_addresses.end()) {
			return sizeof(uint64_t) + (this->function_addresses.size() * sizeof(uint64_t)) + (sizeof(uint64_t) * (found_jump_point - this->jump_addresses.begin()));
		}

		return 0;
	}

	std::pair<void*, uint64_t> create_raw_table(uint64_t default_fallback_address) {
		uint64_t jump_table_size =
			sizeof(uint64_t) +
			(this->function_addresses.size() * sizeof(uint64_t)) +
			(this->jump_addresses.size() * sizeof(uint64_t));
		char* jump_table = new char[jump_table_size] {0};

		size_t index = sizeof(uint64_t);
		for (const auto& function_address : this->function_addresses) {
			copy_uint64_t(jump_table + index, function_address.second);
			index += sizeof(uint64_t);
		}

		for (const auto& jump_address : this->jump_addresses) {
			copy_uint64_t(jump_table + index, std::get<3>(jump_address));
			index += sizeof(uint64_t);
		}

		copy_uint64_t(jump_table, default_fallback_address);
		return { jump_table, jump_table_size };
	}
};

#endif // !JUMP_TABLE_BUILDER_H