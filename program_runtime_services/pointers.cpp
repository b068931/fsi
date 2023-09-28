#include "pch.h"
#include "pointers.h"

return_value inner_allocate(uint64_t size) {
	return fast_call_module<return_value, uint64_t>(
		get_dll_part(),
		index_getter::resm(),
		index_getter::resm_allocate_program_memory(),
		get_current_thread_group_id(),
		size
	);
}
void inner_deallocate(void* pointer) {
	fast_call_module<return_value, void*>(
		get_dll_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_program_memory(),
		get_current_thread_group_id(),
		pointer
	);
}
void inner_check_if_pointer_is_saved(pointer address) {
	unsigned char* saved_variable =
		reinterpret_cast<unsigned char*>(
			fast_call_module(
				get_dll_part(),
				index_getter::excm(),
				index_getter::excm_get_thread_saved_variable()
			)
			);

	if (saved_variable[8] == pointer_return_value) {
		if (std::memcmp(saved_variable, &address, sizeof(pointer)) == 0) {
			pointer nullpointer{};
			std::memcpy(saved_variable, &nullpointer, sizeof(pointer));
		}
	}
}

return_value allocate_pointer(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte, ebyte>(bundle);

	pointer return_address = std::get<0>(arguments);
	byte return_type = std::get<1>(arguments);
	ebyte size = std::get<2>(arguments);

	if (return_type != pointer_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (allocate_pointer)");
		return execution_result_terminate;
	}

	return_value value_pointer_data = inner_allocate(sizeof(uint64_t) * 2); //first 8 bytes - allocated size, second 8 bytes - base address
	char* pointer_data = reinterpret_cast<char*>(value_pointer_data);

	return_value allocated_memory = inner_allocate(size);
	if ((value_pointer_data == reinterpret_cast<return_value>(nullptr)) || (allocated_memory == reinterpret_cast<return_value>(nullptr))) {
		log_program_error(get_dll_part(), "Not enough memory. (allocate_pointer)");
		return execution_result_terminate;
	}

	std::memcpy(pointer_data, &size, sizeof(uint64_t)); //fill in pointer size
	std::memcpy(pointer_data + sizeof(uint64_t), &allocated_memory, sizeof(uint64_t)); //fill in actual pointer value

	std::memcpy(return_address, &value_pointer_data, sizeof(pointer));
	return execution_result_continue;
}
return_value deallocate_pointer(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte>(bundle);
	pointer return_address = std::get<0>(arguments);
	byte return_type = std::get<1>(arguments);

	if (return_type != pointer_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (deallocate_pointer)");
		return execution_result_terminate;
	}

	pointer address{};
	std::memcpy(&address, return_address, sizeof(pointer));
	if (address == 0) return execution_result_continue;

	inner_check_if_pointer_is_saved(address);

	uint64_t base{}; //save pointer's base address
	std::memcpy(&base, reinterpret_cast<char*>(address) + sizeof(uint64_t), sizeof(uint64_t));

	inner_deallocate(address);
	inner_deallocate(reinterpret_cast<void*>(base));

	pointer nullpointer{};
	std::memcpy(return_address, &nullpointer, sizeof(pointer));
	return execution_result_continue;
}
return_value get_allocated_size(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte, pointer>(bundle);
	
	pointer return_address = std::get<0>(arguments);
	byte type = std::get<1>(arguments);
	pointer address = std::get<2>(arguments);

	if (type != ebyte_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (get_allocated_size)");
		return execution_result_terminate;
	}

	uint64_t size{};
	std::memcpy(&size, address, sizeof(uint64_t));

	std::memcpy(return_address, &size, sizeof(ebyte));
	return execution_result_continue;
}