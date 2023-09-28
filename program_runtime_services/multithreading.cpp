#include "pch.h"
#include "multithreading.h"

return_value get_current_thread_group_jump_table_size() {
	std::unique_ptr<arguments_string_element[]> args_string{
		arguments_string_builder::pack<unsigned long long>(get_current_thread_group_id())
	};

	return get_dll_part()->call_module(index_getter::resm(), index_getter::resm_get_jump_table_size(), args_string.get());
}
void* get_current_thread_group_jump_table() {
	std::unique_ptr<arguments_string_element[]> args_string{
		arguments_string_builder::pack<unsigned long long>(get_current_thread_group_id())
	};

	return reinterpret_cast<void*>(get_dll_part()->call_module(index_getter::resm(), index_getter::resm_get_jump_table(), args_string.get()));
}
void* get_function_address(uint64_t function_displacement) {
	char* jump_table_bytes = 
		static_cast<char*>(get_current_thread_group_jump_table());

	void* function_address = nullptr;
	std::memcpy(&function_address, jump_table_bytes + function_displacement, sizeof(void*));
	
	return function_address;
}
return_value check_function_signature(void* function_address, arguments_string_type alleged_signature) {
	std::unique_ptr<arguments_string_element[]> args_string{
		arguments_string_builder::pack<void*, uintptr_t>(alleged_signature, reinterpret_cast<uintptr_t>(function_address))
	};

	return get_dll_part()->call_module(index_getter::progload(), index_getter::progload_check_function_arguments(), args_string.get());
}
bool check_function_displacement(uint64_t function_displacement) {
	return (function_displacement + sizeof(void*)) <= get_current_thread_group_jump_table_size();
}

return_value yield(arguments_string_type) {
	return execution_result_switch;
}
return_value self_terminate(arguments_string_type) {
	return execution_result_terminate;
}
return_value self_priority(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte>(bundle);
	pointer return_address = std::get<0>(arguments);
	byte type = std::get<1>(arguments);

	if (type != ebyte_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (self_priority)");
		return execution_result_terminate;
	}

	return_value priority = fast_call_module(
		get_dll_part(),
		index_getter::excm(),
		index_getter::excm_self_priority()
	);
	
	std::memcpy(return_address, &priority, sizeof(ebyte));
	return execution_result_continue;
}
return_value thread_id(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte>(bundle);
	pointer return_address = std::get<0>(arguments);
	byte type = std::get<1>(arguments);

	if (type != ebyte_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (thread_id)");
		return execution_result_terminate;
	}

	return_value thread_id = fast_call_module(
		get_dll_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_id()
	);
	
	std::memcpy(return_address, &thread_id, sizeof(ebyte));
	return execution_result_continue;
}
return_value thread_group_id(arguments_string_type bundle) {
	auto arguments = arguments_string_builder::unpack<pointer, byte>(bundle);
	pointer return_address = std::get<0>(arguments);
	byte type = std::get<1>(arguments);

	if (type != ebyte_return_value) {
		log_program_error(get_dll_part(), "Incorrect return type. (thread_group_id)");
		return execution_result_terminate;
	}

	uint64_t thread_group_id = get_current_thread_group_id();
	std::memcpy(return_address, &thread_group_id, sizeof(ebyte));

	return execution_result_continue;
}
return_value dynamic_call(arguments_string_type bundle) {
	arguments_array_type arguments =
		arguments_string_builder::convert_to_arguments_array(bundle);

	ebyte function_displacement{};
	if (!arguments_string_builder::extract_value_from_arguments_array<ebyte>(&function_displacement, 0, arguments)) {
		log_program_error(get_dll_part(), "Dynamic call incorrect structure.");
		return execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<arguments_string_type, size_t> args_string_size =
			arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

		std::unique_ptr<arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		return_value result = fast_call_module<void*, void*>(
			get_dll_part(),
			index_getter::excm(),
			index_getter::excm_dynamic_call(),
			get_function_address(function_displacement),
			thread_main_parameters.get()
		);

		if (result != 0) {
			log_program_error(get_dll_part(), "Dynamic call failed.");
			return execution_result_terminate;
		}
	}

	return execution_result_continue;
}

return_value create_thread(arguments_string_type bundle) {
	arguments_array_type arguments = 
		arguments_string_builder::convert_to_arguments_array(bundle);

	ebyte priority{};
	ebyte function_displacement{};
	if (
		!arguments_string_builder::extract_value_from_arguments_array<ebyte>(&priority, 0, arguments) ||
			!arguments_string_builder::extract_value_from_arguments_array<ebyte>(&function_displacement, 1, arguments)
	) {
		log_program_error(get_dll_part(), "Incorrect create_thread call structure.");
		return execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<arguments_string_type, size_t> args_string_size = 
			arguments_string_builder::convert_from_arguments_array(arguments.begin() + 2, arguments.end());

		std::unique_ptr<arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		return_value result = fast_call_module<return_value, void*, void*, uint64_t>(
			get_dll_part(),
			index_getter::excm(),
			index_getter::excm_create_thread(),
			priority,
			get_function_address(function_displacement),
			thread_main_parameters.get(),
			args_string_size.second
		);

		if (result != 0) {
			log_program_error(get_dll_part(), "Can not create a new thread.");
			return execution_result_terminate;
		}
	}

	return execution_result_continue;
}
return_value create_thread_group(arguments_string_type bundle) {
	arguments_array_type arguments =
		arguments_string_builder::convert_to_arguments_array(bundle);

	ebyte function_displacement{};
	if (!arguments_string_builder::extract_value_from_arguments_array<ebyte>(&function_displacement, 0, arguments)) {
		log_program_error(get_dll_part(), "Incorrect create_thread_group call structure.");
		return execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<arguments_string_type, size_t> args_string_size =
			arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

		std::unique_ptr<arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		return_value result = fast_call_module<void*, void*, uint64_t>(
			get_dll_part(),
			index_getter::excm(),
			index_getter::excm_self_duplicate(),
			get_function_address(function_displacement),
			thread_main_parameters.get(),
			args_string_size.second
		);

		if (result != 0) {
			log_program_error(get_dll_part(), "Can not create a new thread group.");
			return execution_result_terminate;
		}
	}

	return execution_result_continue;
}