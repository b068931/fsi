#include "pch.h"
#include "module_interoperation.h"
#include "execution_module.h"
#include "program_state_manager.h"
#include "executions_backend_functions.h"
#include "thread_local_structure.h"
#include "../module_mediator/fsi_types.h"
#include "../module_mediator/module_part.h"
#include "../program_loader/program_functions.h"
#include "../logger_module/logging.h"

module_mediator::return_value on_thread_creation(module_mediator::arguments_string_type bundle) {
	auto [container_id, thread_id, preferred_stack_size] =
		module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::return_value, std::uint64_t>(bundle);

	thread_local_structure* thread_structure = get_thread_local_structure();

	char* thread_state_memory = inner_allocate_thread_memory(thread_id, program_state_manager::thread_state_size);
	char* thread_stack_memory = inner_allocate_thread_memory(thread_id, preferred_stack_size);
	
	char* thread_stack_end = thread_stack_memory + preferred_stack_size - (sizeof(module_mediator::one_byte) + sizeof(std::uint64_t));

	//get program jump table from resm
	void* program_jump_table = reinterpret_cast<void*>(
		module_mediator::fast_call<module_mediator::return_value>(
			get_module_part(),
			index_getter::resm(),
			index_getter::resm_get_jump_table(),
			container_id
		)
	);

	//fill in thread_state - start
	inner_fill_in_reg_array_entry( //function address
		1, 
		thread_state_memory, 
		reinterpret_cast<std::uintptr_t>(thread_structure->program_function_address)
	);

	inner_fill_in_reg_array_entry( //jump table
		2,
		thread_state_memory,
		reinterpret_cast<std::uintptr_t>(program_jump_table)
	);

	inner_fill_in_reg_array_entry( //thread state
		3,
		thread_state_memory,
		reinterpret_cast<std::uintptr_t>(thread_state_memory)
	);

	std::uintptr_t result = inner_initialize_thread_stack(thread_structure->program_function_address, thread_stack_memory, thread_stack_end, thread_id);
	if (result == reinterpret_cast<std::uintptr_t>(nullptr)) {
		LOG_PROGRAM_ERROR(get_module_part(), "Thread stack initialization has failed.");

		module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
			get_module_part(),
			index_getter::resm(),
			index_getter::resm_deallocate_thread_memory(),
			thread_id,
			thread_state_memory
		);
		
		module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
			get_module_part(),
			index_getter::resm(),
			index_getter::resm_deallocate_thread_memory(),
			thread_id,
			thread_stack_memory
		);

		module_mediator::return_value result_container_id = inner_deallocate_thread(thread_id);
		if (inner_get_container_running_threads_count(result_container_id) == 0) {
			get_thread_manager().forget_thread_group(result_container_id);
			inner_deallocate_program_container(result_container_id);
		}

		return module_mediator::module_failure;
	}
	
	inner_fill_in_reg_array_entry( //stack current position
		4,
		thread_state_memory,
		result
	);

	inner_fill_in_reg_array_entry( //stack end
		5,
		thread_state_memory,
		reinterpret_cast<std::uintptr_t>(thread_stack_end) //account for the space that will be used to save the state of one variable between function calls
	);

	inner_fill_in_reg_array_entry( //program control functions
		6,
		thread_state_memory,
		reinterpret_cast<std::uintptr_t>(get_program_control_functions_addresses())
	);
	//fill in thread state - end

	get_thread_manager().add_thread(
		container_id,
		thread_id,
		thread_structure->priority,
		thread_state_memory,
		program_jump_table
	);

	LOG_PROGRAM_INFO(get_module_part(), "New thread has been successfully created.");
	return module_mediator::module_success;
}
module_mediator::return_value on_container_creation(module_mediator::arguments_string_type bundle) {
	auto [container_id, program_main, preferred_stack_size] = 
		module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, std::uint64_t>(bundle);

	get_thread_manager().add_thread_group(container_id, preferred_stack_size);
	LOG_PROGRAM_INFO(get_module_part(), "New thread group has been successfully created.");

	return inner_create_thread(container_id, 0, program_main);
}
module_mediator::return_value register_deferred_callback(module_mediator::arguments_string_type bundle) {
	auto [callback_info] = 
		module_mediator::arguments_string_builder::unpack<module_mediator::memory>(bundle);

	module_mediator::callback_bundle* callback = static_cast<module_mediator::callback_bundle*>(callback_info);
	get_thread_local_structure()->deferred_callbacks.push_back(callback);

	return module_mediator::module_success;
}

module_mediator::return_value self_duplicate(module_mediator::arguments_string_type bundle) {
	auto [main_function_address, main_function_parameters, parameters_size] =
		module_mediator::arguments_string_builder::unpack<void*, void*, std::uint64_t>(bundle);

	module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size]{};
	std::memcpy(copy, main_function_parameters, parameters_size);

	return inner_self_duplicate(main_function_address, copy);
}
module_mediator::return_value self_priority(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.priority;
}
module_mediator::return_value get_thread_saved_variable(module_mediator::arguments_string_type) {
	char* thread_state = static_cast<char*>(get_thread_local_structure()->currently_running_thread_information.thread_state) + 40;
	module_mediator::memory thread_stack_end{};

	std::memcpy(static_cast<void*>(&thread_stack_end), thread_state, sizeof(module_mediator::memory));
	return reinterpret_cast<std::uintptr_t>(thread_stack_end);
}
module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
	auto [function_address, function_arguments_data] =
		module_mediator::arguments_string_builder::unpack<void*, void*>(bundle);

	module_mediator::arguments_string_type function_arguments_string =
        static_cast<module_mediator::arguments_string_type>(function_arguments_data);

	if (inner_check_function_signature(function_address, function_arguments_string) != module_mediator::module_success) {
		LOG_PROGRAM_ERROR(
			get_module_part(), "A function signature for function '" + get_exposed_function_name(function_address) +
			"' does not match passed arguments. (dynamic call)"
		);

		return module_mediator::module_failure;
	}

	program_state_manager state_manager{ 
		static_cast<char*>(get_thread_local_structure()->currently_running_thread_information.thread_state) 
	};

	std::uintptr_t program_return_address = state_manager.get_return_address();
	module_mediator::arguments_array_type function_arguments = module_mediator::arguments_string_builder::convert_to_arguments_array(
		function_arguments_string
	);

	function_arguments.insert(
		function_arguments.cbegin(),
		{ static_cast<module_mediator::arguments_string_element>(module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>), &program_return_address }
	);

	std::uintptr_t new_current_stack_position = inner_apply_initializer_on_thread_stack(
		reinterpret_cast<char*>(state_manager.get_current_stack_position()),
		reinterpret_cast<char*>(state_manager.get_stack_end()),
		function_arguments
	);

	if (new_current_stack_position == reinterpret_cast<std::uintptr_t>(nullptr)) {
		LOG_PROGRAM_ERROR(get_module_part(), "The thread stack initialization has failed. (dynamic call)");
		return module_mediator::module_failure;
	}

	state_manager.set_current_stack_position(
		new_current_stack_position - module_mediator::arguments_string_builder::get_type_size_by_index(module_mediator::arguments_string_builder::get_type_index<std::uintptr_t>)
	);
	
	state_manager.set_return_address(
		reinterpret_cast<std::uintptr_t>(static_cast<char*>(function_address) + function_save_return_address_size)
	);

	return module_mediator::module_success;
}

module_mediator::return_value get_current_thread_id(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.thread_id;
}
module_mediator::return_value get_current_thread_group_id(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.thread_group_id;
}
module_mediator::return_value make_runnable(module_mediator::arguments_string_type bundle) {
	auto [thread_id] = 
		module_mediator::arguments_string_builder::unpack<module_mediator::return_value>(bundle);

	if (get_thread_manager().make_runnable(thread_id)) {
        LOG_PROGRAM_INFO(get_module_part(), "Made a thread with id '" + std::to_string(thread_id) + "' runnable again.");
	}
	else {
		LOG_PROGRAM_WARNING(
			get_module_part(),
			std::format(
				"Was unable to make a thread with {} runnable again. This most likely indicates a data race.",
                thread_id
			)
		);

		return module_mediator::module_failure;
	}

	return module_mediator::module_success;
}
module_mediator::return_value start(module_mediator::arguments_string_type bundle) {
	auto [thread_count] =
		module_mediator::arguments_string_builder::unpack<std::uint16_t>(bundle);

	LOG_INFO(
		get_module_part(), 
		"Creating execution daemons. Count: " + std::to_string(thread_count)
	);

	get_thread_manager().startup(thread_count);
	return module_mediator::module_success;
}
module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
	auto [priority, main_function_address, main_function_parameters, parameters_size] =
		module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*, std::uint64_t>(bundle);

	module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[parameters_size]{};
	std::memcpy(copy, main_function_parameters, parameters_size);

	return inner_create_thread_with_initializer(priority, main_function_address, copy);
}
