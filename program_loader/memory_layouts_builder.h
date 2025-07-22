#ifndef MEMORY_LAYOUTS_BUILDER_H
#define MEMORY_LAYOUTS_BUILDER_H

#include "pch.h"
#include "variable_with_id.h" //entity_id

class memory_layouts_builder {
public:
	enum class variable_size : std::uint8_t {
		BYTE_SIZE = 1,
		TWO_BYTES_SIZE = 2,
		FOUR_BYTES_SIZE = 4,
		EIGHT_BYTES_SIZE = 8,
		POINTER_SIZE = 8,
		UNKNOWN = 0
	};

	static variable_size get_variable_size(std::uint8_t variable_type) {
		variable_size size;
		switch (variable_type) {
            case 0: {
                size = variable_size::BYTE_SIZE;
                break;
            }
            case 1: {
                size = variable_size::TWO_BYTES_SIZE;
                break;
            }
            case 2: {
                size = variable_size::FOUR_BYTES_SIZE;
                break;
            }
            case 3: {
                size = variable_size::EIGHT_BYTES_SIZE;
                break;
            }
            case 4: {
                size = variable_size::POINTER_SIZE;
                break;
            }
            default: {
                size = variable_size::UNKNOWN;
            }
		}

		return size;
	}

	using memory_addresses = std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>;
	static memory_layouts_builder::memory_addresses merge_memory_layouts(
		std::pair<memory_layouts_builder::memory_addresses, memory_layouts_builder::memory_addresses>& arguments_locals_pair
	) {
		memory_layouts_builder::memory_addresses temp{};

		temp.merge(arguments_locals_pair.first);
		temp.merge(arguments_locals_pair.second);

		return temp;
	}

private:
	std::vector<
		std::pair<std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>, std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>>
	> memory_layouts;

	std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>* current_map{};
	std::int32_t variable_relative_address = 0;
public:
	std::vector<
		std::pair<std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>, std::map<entity_id, std::pair<std::int32_t, std::uint8_t>>>
	> get_memory_layouts() {
		return std::move(this->memory_layouts);
	}
	void add_new_function() {
		this->variable_relative_address = 0;
		this->memory_layouts.emplace_back();

		this->current_map = &this->memory_layouts.back().first;
	}
	void move_to_function_arguments() { 
		this->variable_relative_address += 8; /*functions return address will be stored between function arguments function locals*/
		this->current_map = &this->memory_layouts.back().second;
	}
	void add_new_variable(const std::pair<entity_id, std::uint8_t>& variable_info) {
		variable_size var_size = get_variable_size(variable_info.second);
		if (var_size != variable_size::UNKNOWN) {
			this->variable_relative_address += static_cast<std::uint8_t>(var_size);
			(*this->current_map)[variable_info.first] = { -this->variable_relative_address, variable_info.second };
		}
	}
};

#endif // !MEMORY_LAYOUTS_BUILDER_H