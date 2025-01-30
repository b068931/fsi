#include "pch.h"
#include "multithreading.h"

module_mediator::return_value get_current_thread_group_jump_table_size() {
	std::unique_ptr<module_mediator::arguments_string_element[]> args_string{
		module_mediator::arguments_string_builder::pack<unsigned long long>(get_current_thread_group_id())
	};

	return get_module_part()->call_module(index_getter::resm(), index_getter::resm_get_jump_table_size(), args_string.get());
}
void* get_current_thread_group_jump_table() {
	std::unique_ptr<module_mediator::arguments_string_element[]> args_string{
		module_mediator::arguments_string_builder::pack<unsigned long long>(get_current_thread_group_id())
	};

	return reinterpret_cast<void*>(get_module_part()->call_module(index_getter::resm(), index_getter::resm_get_jump_table(), args_string.get()));
}
void* get_function_address(std::uint64_t function_displacement) {
	char* jump_table_bytes = 
		static_cast<char*>(get_current_thread_group_jump_table());

	void* function_address = nullptr;
	std::memcpy(&function_address, jump_table_bytes + function_displacement, sizeof(void*));
	
	return function_address;
}
module_mediator::return_value check_function_signature(void* function_address, module_mediator::arguments_string_type alleged_signature) {
	std::unique_ptr<module_mediator::arguments_string_element[]> args_string{
		module_mediator::arguments_string_builder::pack<void*, uintptr_t>(alleged_signature, reinterpret_cast<uintptr_t>(function_address))
	};

	return get_module_part()->call_module(index_getter::progload(), index_getter::progload_check_function_arguments(), args_string.get());
}
bool check_function_displacement(std::uint64_t function_displacement) {
	return (function_displacement + sizeof(void*)) <= get_current_thread_group_jump_table_size();
}

module_mediator::return_value yield(module_mediator::arguments_string_type) {
	return module_mediator::execution_result_switch;
}
module_mediator::return_value self_terminate(module_mediator::arguments_string_type) {
	return module_mediator::execution_result_terminate;
}
module_mediator::return_value self_priority(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte>(bundle);
	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte type = std::get<1>(arguments);

	if (type != module_mediator::eight_bytes_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (self_priority)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::return_value priority = module_mediator::fast_call(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_self_priority()
	);
	
	std::memcpy(return_address, &priority, sizeof(module_mediator::eight_bytes));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value thread_id(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte>(bundle);
	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte type = std::get<1>(arguments);

	if (type != module_mediator::eight_bytes_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (thread_id)");
		return module_mediator::execution_result_terminate;
	}

	module_mediator::return_value thread_id = module_mediator::fast_call(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_id()
	);
	
	std::memcpy(return_address, &thread_id, sizeof(module_mediator::eight_bytes));
	return module_mediator::execution_result_continue;
}
module_mediator::return_value thread_group_id(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::pointer, module_mediator::one_byte>(bundle);
	module_mediator::pointer return_address = std::get<0>(arguments);
	module_mediator::one_byte type = std::get<1>(arguments);

	if (type != module_mediator::eight_bytes_return_value) {
		log_program_error(get_module_part(), "Incorrect return type. (thread_group_id)");
		return module_mediator::execution_result_terminate;
	}

	std::uint64_t thread_group_id = get_current_thread_group_id();
	std::memcpy(return_address, &thread_group_id, sizeof(module_mediator::eight_bytes));

	return module_mediator::execution_result_continue;
}
module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
	module_mediator::arguments_array_type arguments =
		module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

	module_mediator::eight_bytes function_displacement{};
	if (!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 0, arguments)) {
		log_program_error(get_module_part(), "Dynamic call incorrect structure.");
		return module_mediator::execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size =
			module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

		std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		module_mediator::return_value result = module_mediator::fast_call<void*, void*>(
			get_module_part(),
			index_getter::excm(),
			index_getter::excm_dynamic_call(),
			get_function_address(function_displacement),
			thread_main_parameters.get()
		);

		if (result != 0) {
			log_program_error(get_module_part(), "Dynamic call failed.");
			return module_mediator::execution_result_terminate;
		}
	}

	return module_mediator::execution_result_continue;
}

module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
	module_mediator::arguments_array_type arguments = 
		module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

	module_mediator::eight_bytes priority{};
	module_mediator::eight_bytes function_displacement{};
	if (
		!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&priority, 0, arguments) ||
			!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 1, arguments)
	) {
		log_program_error(get_module_part(), "Incorrect create_thread call structure.");
		return module_mediator::execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size = 
			module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 2, arguments.end());

		std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		module_mediator::return_value result = module_mediator::fast_call<module_mediator::return_value, void*, void*, std::uint64_t>(
			get_module_part(),
			index_getter::excm(),
			index_getter::excm_create_thread(),
			priority,
			get_function_address(function_displacement),
			thread_main_parameters.get(),
			args_string_size.second
		);

		if (result != 0) {
			log_program_error(get_module_part(), "Can not create a new thread.");
			return module_mediator::execution_result_terminate;
		}
	}

	return module_mediator::execution_result_continue;
}
module_mediator::return_value create_thread_group(module_mediator::arguments_string_type bundle) {
	module_mediator::arguments_array_type arguments =
		module_mediator::arguments_string_builder::convert_to_arguments_array(bundle);

	module_mediator::eight_bytes function_displacement{};
	if (!module_mediator::arguments_string_builder::extract_value_from_arguments_array<module_mediator::eight_bytes>(&function_displacement, 0, arguments)) {
		log_program_error(get_module_part(), "Incorrect create_thread_group call structure.");
		return module_mediator::execution_result_terminate;
	}

	if (check_function_displacement(function_displacement)) {
		std::pair<module_mediator::arguments_string_type, std::size_t> args_string_size =
			module_mediator::arguments_string_builder::convert_from_arguments_array(arguments.begin() + 1, arguments.end());

		std::unique_ptr<module_mediator::arguments_string_element[]> thread_main_parameters{
			args_string_size.first
		};

		module_mediator::return_value result = module_mediator::fast_call<void*, void*, std::uint64_t>(
			get_module_part(),
			index_getter::excm(),
			index_getter::excm_self_duplicate(),
			get_function_address(function_displacement),
			thread_main_parameters.get(),
			args_string_size.second
		);

		if (result != 0) {
			log_program_error(get_module_part(), "Can not create a new thread group.");
			return module_mediator::execution_result_terminate;
		}
	}

	return module_mediator::execution_result_continue;
}