#include "pch.h"
#include "pointers.h"

namespace {
	module_mediator::return_value allocate(std::uint64_t size) {
		return module_mediator::fast_call<module_mediator::return_value, std::uint64_t>(
			get_module_part(),
			index_getter::resm(),
			index_getter::resm_allocate_program_memory(),
			get_current_thread_group_id(),
			size
		);
	}
	void deallocate(void* pointer) {
		module_mediator::fast_call<module_mediator::return_value, void*>(
			get_module_part(),
			index_getter::resm(),
			index_getter::resm_deallocate_program_memory(),
			get_current_thread_group_id(),
			pointer
		);
	}
	void check_if_pointer_is_saved(module_mediator::memory address) {
		unsigned char* saved_variable =
			reinterpret_cast<unsigned char*>(
				module_mediator::fast_call(
					get_module_part(),
					index_getter::excm(),
					index_getter::excm_get_thread_saved_variable()
				)
				);

		if (saved_variable[8] == module_mediator::pointer_return_value) {
			if (std::memcmp(saved_variable, &address, sizeof(module_mediator::memory)) == 0) {
				module_mediator::memory nullpointer{};
				std::memcpy(saved_variable, &nullpointer, sizeof(module_mediator::memory));
			}
		}
	}
}

module_mediator::return_value allocate_memory(module_mediator::arguments_string_type bundle) {
	auto [return_address, return_type, size] =
		module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte, module_mediator::eight_bytes>(bundle);

	if (return_type != module_mediator::pointer_return_value) {
		LOG_PROGRAM_ERROR(get_module_part(), "Incorrect return type. (allocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::return_value value_pointer_data = allocate(sizeof(std::uint64_t) * 2); //first 8 bytes - allocated size, second 8 bytes - base address
	char* pointer_data = reinterpret_cast<char*>(value_pointer_data);

	module_mediator::return_value allocated_memory = allocate(size);
	if ((value_pointer_data == reinterpret_cast<module_mediator::return_value>(nullptr)) || (allocated_memory == reinterpret_cast<module_mediator::return_value>(nullptr))) {
		LOG_PROGRAM_ERROR(get_module_part(), "Not enough memory. (allocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	std::memcpy(pointer_data, &size, sizeof(std::uint64_t)); //fill in memory size
	std::memcpy(pointer_data + sizeof(std::uint64_t), &allocated_memory, sizeof(std::uint64_t)); //fill in actual memory value

	std::memcpy(return_address, &value_pointer_data, sizeof(module_mediator::memory));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value deallocate_memory(module_mediator::arguments_string_type bundle) {
	auto [return_address, return_type] =
		module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte>(bundle);

	if (return_type != module_mediator::pointer_return_value) {
		LOG_PROGRAM_ERROR(get_module_part(), "Incorrect return type. (deallocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::memory address{};
	std::memcpy(&address, return_address, sizeof(module_mediator::memory));
	if (address == nullptr) return module_mediator::execution_result_continue;

	check_if_pointer_is_saved(address);

	std::uint64_t base{}; //save memory's base address
	std::memcpy(&base, static_cast<char*>(address) + sizeof(std::uint64_t), sizeof(std::uint64_t));

	deallocate(address);
	deallocate(reinterpret_cast<void*>(base));

	module_mediator::memory nullpointer{};
	std::memcpy(return_address, &nullpointer, sizeof(module_mediator::memory));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value get_allocated_size(module_mediator::arguments_string_type bundle) {
	auto [return_address, type, address] =
		module_mediator::arguments_string_builder::unpack<module_mediator::memory, module_mediator::one_byte, module_mediator::memory>(bundle);
	
	if (type != module_mediator::eight_bytes_return_value) {
		LOG_PROGRAM_ERROR(get_module_part(), "Incorrect return type. (get_allocated_size)");
		return module_mediator::execution_result_terminate;
	}

	std::uint64_t size{};
	std::memcpy(&size, address, sizeof(std::uint64_t));

	std::memcpy(return_address, &size, sizeof(module_mediator::eight_bytes));
	return module_mediator::execution_result_continue;
}