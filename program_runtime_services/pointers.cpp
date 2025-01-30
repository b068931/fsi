#include "pch.h"
#include "pointers.h"

module_mediator::return_value inner_allocate(uint64_t size) {
	return module_mediator::fast_call<module_mediator::return_value, uint64_t>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_allocate_program_memory(),
		get_current_thread_group_id(),
		size
	);
}
void inner_deallocate(void* pointer) {
	module_mediator::fast_call<module_mediator::return_value, void*>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_program_memory(),
		get_current_thread_group_id(),
		pointer
	);
}
void inner_check_if_pointer_is_saved(module_mediator::pointer address) {
	unsigned char* saved_variable =
		reinterpret_cast<unsigned char*>(
			module_mediator::fast_call(
				get_module_part(),
				index_getter::excm(),
				index_getter::excm_get_thread_saved_variable()
			)
			);

	if (saved_variable[8] == module_mediator::pointer_return_value) {
		if (std::memcmp(saved_variable, &address, sizeof(module_mediator::pointer)) == 0) {
			module_mediator::pointer nullpointer{};
			std::memcpy(saved_variable, &nullpointer, sizeof(module_mediator::pointer));
		}
	}
}

module_mediator::return_value allocate_pointer(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte, module_mediator::eight_bytes>(bundle);

	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte return_type = std::get<1>(arguments);
	module_mediator::eight_bytes size = std::get<2>(arguments);

	if (return_type != module_mediator::pointer_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (allocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::return_value value_pointer_data = inner_allocate(sizeof(uint64_t) * 2); //first 8 bytes - allocated size, second 8 bytes - base address
	char* pointer_data = reinterpret_cast<char*>(value_pointer_data);

	module_mediator::return_value allocated_memory = inner_allocate(size);
	if ((value_pointer_data == reinterpret_cast<module_mediator::return_value>(nullptr)) || (allocated_memory == reinterpret_cast<module_mediator::return_value>(nullptr))) {
		log_program_error(get_module_part(), "Not enough memory. (allocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	std::memcpy(pointer_data, &size, sizeof(uint64_t)); //fill in pointer size
	std::memcpy(pointer_data + sizeof(uint64_t), &allocated_memory, sizeof(uint64_t)); //fill in actual pointer value

	std::memcpy(return_address, &value_pointer_data, sizeof(module_mediator::pointer));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value deallocate_pointer(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte>(bundle);
	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte return_type = std::get<1>(arguments);

	if (return_type != module_mediator::pointer_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (deallocate_pointer)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::pointer address{};
	std::memcpy(&address, return_address, sizeof(module_mediator::pointer));
	if (address == 0) return module_mediator::execution_result_continue;

	inner_check_if_pointer_is_saved(address);

	uint64_t base{}; //save pointer's base address
	std::memcpy(&base, reinterpret_cast<char*>(address) + sizeof(uint64_t), sizeof(uint64_t));

	inner_deallocate(address);
	inner_deallocate(reinterpret_cast<void*>(base));

	module_mediator::pointer nullpointer{};
	std::memcpy(return_address, &nullpointer, sizeof(module_mediator::pointer));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value get_allocated_size(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte, module_mediator::pointer>(bundle);
	
	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte type = std::get<1>(arguments);
	module_mediator::pointer address = std::get<2>(arguments);

	if (type != module_mediator::eight_bytes_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (get_allocated_size)");
		return module_mediator::execution_result_terminate;
	}

	uint64_t size{};
	std::memcpy(&size, address, sizeof(uint64_t));

	std::memcpy(return_address, &size, sizeof(module_mediator::eight_bytes));
	return module_mediator::execution_result_continue;
}