#include "pch.h"
#include "thread_local_structure.h"
#include "module_interoperation.h"
#include "thread_manager.h"
#include "assembly_functions.h"
#include "program_state_manager.h"
#include "executions_backend_functions.h"
#include "../logger_module/logging.h"
#include "../module_mediator/module_part.h"
#include "../module_mediator/fsi_types.h"

module_mediator::return_value inner_deallocate_thread(module_mediator::return_value thread_id) {
	return module_mediator::fast_call<module_mediator::return_value>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_thread(),
		thread_id
	);
}

void inner_deallocate_program_container(module_mediator::return_value container_id) {
	module_mediator::fast_call<module_mediator::return_value>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_program_container(),
		container_id
	);
}

void inner_delete_running_thread() {
	thread_local_structure* thread_structure = get_thread_local_structure();

	module_mediator::return_value thread_id = thread_structure->currently_running_thread_information.thread_id;
	module_mediator::return_value thread_group_id = thread_structure->currently_running_thread_information.thread_group_id;
	void* thread_state = thread_structure->currently_running_thread_information.thread_state;
	std::uint64_t preferred_stack_size = thread_structure->currently_running_thread_information.preferred_stack_size;

	program_state_manager program_state_manager{ 
		static_cast<char*>(thread_state) 
	};

	module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_thread_memory(),
		thread_id,
		reinterpret_cast<void*>(program_state_manager.get_stack_start(preferred_stack_size))
	);

	module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_deallocate_thread_memory(),
		thread_id,
		thread_state
	);

	bool is_delete_container = get_thread_manager().delete_thread(thread_group_id, thread_id);

	LOG_INFO(get_module_part(), "Deallocating a thread. ID: " + std::to_string(thread_id));
	inner_deallocate_thread(thread_id);
	if (is_delete_container) {
		LOG_INFO(get_module_part(), "Deallocating a thread group. ID: " + std::to_string(thread_group_id));
		inner_deallocate_program_container(thread_group_id);
	}
}

void program_resume() {
	get_thread_local_structure()->currently_running_thread_information.state = scheduler::thread_states::running;
}

void thread_terminate() {
	LOG_PROGRAM_INFO(get_module_part(), "Thread was terminated.");

	get_thread_local_structure()->currently_running_thread_information.put_back_structure = nullptr;
	inner_delete_running_thread();
}

[[noreturn]] void inner_call_module_error(module_mediator::module_part::call_error error) {
	switch (error) {
	case module_mediator::module_part::call_error::function_is_not_visible:
		LOG_PROGRAM_ERROR(get_module_part(), "Called module function is not visible. Thread terminated.");
		break;

	case module_mediator::module_part::call_error::invalid_arguments_string:
		LOG_PROGRAM_ERROR(get_module_part(), "Incorrect arguments were used for the module function call. Thread terminated.");
		break;

	case module_mediator::module_part::call_error::unknown_index:
		LOG_PROGRAM_ERROR(get_module_part(), "Module function does not exist. Thread terminated.");
		break;

	case module_mediator::module_part::call_error::no_error: break;
	}

	thread_terminate();
	load_execution_thread(get_thread_local_structure()->execution_thread_state);
}

[[noreturn]] void inner_call_module(std::uint64_t module_id, std::uint64_t function_id, module_mediator::arguments_string_type args_string) {
	module_mediator::return_value action_code = get_module_part()->call_module_visible_only(module_id, function_id, args_string, &inner_call_module_error);
	
	//all these functions are declared as [[noreturn]]
	switch (action_code)
	{
	case module_mediator::execution_result_continue:
		program_resume();
		resume_program_execution(get_thread_local_structure()->currently_running_thread_information.thread_state);

	case module_mediator::execution_result_switch:
		load_execution_thread(get_thread_local_structure()->execution_thread_state);

	case module_mediator::execution_result_terminate:
		LOG_PROGRAM_INFO(get_module_part(), "Requested thread termination.");

		thread_terminate();
		load_execution_thread(get_thread_local_structure()->execution_thread_state);

	case module_mediator::execution_result_block:
		LOG_PROGRAM_INFO(get_module_part(), "Was blocked.");

	    get_thread_manager().block(get_thread_local_structure()->currently_running_thread_information.thread_id);
		load_execution_thread(get_thread_local_structure()->execution_thread_state);

	default:
		LOG_PROGRAM_FATAL(get_module_part(), "Incorrect return code. Process will be killed with abort.");
		std::abort();
	}
}

module_mediator::return_value inner_self_duplicate(void* main_function, module_mediator::arguments_string_type initializer) {
	assert(get_thread_local_structure()->initializer == nullptr && "possible memory leak");
	get_thread_local_structure()->initializer = initializer;
	if (initializer != nullptr) { //you can not initialize a thread_group with memory because this can lead to data race
		constexpr auto pointer_type_index = module_mediator::arguments_string_builder::get_type_index<module_mediator::memory>;
		
		module_mediator::arguments_string_element arguments_count = initializer[0];
		module_mediator::arguments_string_type arguments_types = initializer + 1;
		for (module_mediator::arguments_string_element index = 0; index < arguments_count; ++index) {
			if (arguments_types[index] == static_cast<module_mediator::arguments_string_element>(pointer_type_index)) {
				delete[] initializer;
				get_thread_local_structure()->initializer = nullptr;

				LOG_PROGRAM_ERROR(
					get_module_part(), 
					"Shared memory between thread groups is not allowed. Thread group cannot depend on another thread group."
				);

				return module_mediator::module_failure;
			}
		}
	}

	return module_mediator::fast_call<module_mediator::return_value, module_mediator::memory>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_duplicate_container(),
		get_thread_local_structure()->currently_running_thread_information.thread_group_id,
		main_function
	);
}

module_mediator::return_value inner_get_container_running_threads_count(module_mediator::return_value container_id) {
	return module_mediator::fast_call<module_mediator::return_value>(
		get_module_part(),
		index_getter::resm(),
		index_getter::resm_get_running_threads_count(),
		container_id
	);
}

void inner_fill_in_reg_array_entry(std::uint64_t entry_index, char* memory, std::uint64_t value) {
	std::memcpy(memory + (sizeof(std::uint64_t) * entry_index), &value, sizeof(std::uint64_t));
}

module_mediator::return_value inner_create_thread(module_mediator::return_value thread_group_id, module_mediator::return_value priority, void* function_address) {
	thread_local_structure* thread_structure = get_thread_local_structure();
	LOG_PROGRAM_INFO(get_module_part(), "Creating a new thread.");

	thread_structure->program_function_address = function_address;
	thread_structure->priority = priority;

	return module_mediator::fast_call<module_mediator::return_value>(
		::get_module_part(),
		index_getter::resm(),
		index_getter::resm_create_new_thread(),
		thread_group_id
	);
}

module_mediator::return_value inner_create_thread_with_initializer(module_mediator::return_value priority, void* function_address, module_mediator::arguments_string_type initializer) {
	assert(get_thread_local_structure()->initializer == nullptr && "possible memory leak");
	get_thread_local_structure()->initializer = initializer;
	return inner_create_thread(
		get_thread_local_structure()->currently_running_thread_information.thread_group_id,
		priority,
		function_address
	);
}

char* inner_allocate_thread_memory(module_mediator::return_value thread_id, std::uint64_t size) {
	return reinterpret_cast<char*>(
		module_mediator::fast_call<module_mediator::return_value, module_mediator::eight_bytes>(
			::get_module_part(),
			index_getter::resm(),
			index_getter::resm_allocate_thread_memory(),
			thread_id,
			size
		)
	);
}

module_mediator::return_value inner_check_function_signature(void* function_address, module_mediator::arguments_string_type initializer) {
	std::unique_ptr<module_mediator::arguments_string_element[]> default_signature{ module_mediator::arguments_string_builder::get_types_string() };
	module_mediator::arguments_string_type alleged_signature = default_signature.get();
	if (initializer != nullptr) {
		alleged_signature = initializer;
	}

	return module_mediator::fast_call<module_mediator::memory, std::uintptr_t>(
		::get_module_part(),
		index_getter::progload(),
		index_getter::progload_check_function_arguments(),
		alleged_signature,
		reinterpret_cast<std::uintptr_t>(function_address)
	);
}

[[maybe_unused]] std::string get_exposed_function_name(void* function_address) {
	module_mediator::return_value functions_symbols_address = module_mediator::fast_call(
		::get_module_part(),
		index_getter::progload(),
		index_getter::progload_get_function_name(),
		reinterpret_cast<std::uintptr_t>(function_address)
	);

	std::string function_name{ "[UNKNOWN_FUNCTION_NAME]" };
	char* function_symbols = reinterpret_cast<char*>(functions_symbols_address);
	if (function_symbols != nullptr) {
		function_name = function_symbols;
	}

	return function_name;
}

std::uintptr_t inner_apply_initializer_on_thread_stack(char* thread_stack_memory, char* thread_stack_end, const module_mediator::arguments_array_type& arguments) {
	for (auto argument : std::ranges::reverse_view(arguments)) {
		std::size_t type_size = module_mediator::arguments_string_builder::get_type_size_by_index(argument.first);
		if ((thread_stack_memory + type_size) >= thread_stack_end) {
			LOG_PROGRAM_ERROR(::get_module_part(), "Not enough stack space to initialize thread stack.");
			return reinterpret_cast<std::uintptr_t>(nullptr);
		}

		std::memcpy(
			thread_stack_memory,
            argument.second,
			type_size
		);

		thread_stack_memory += type_size;
	}

	return reinterpret_cast<std::uintptr_t>(thread_stack_memory);
}

std::uintptr_t inner_initialize_thread_stack(void* function_address, char* thread_stack_memory, char* thread_stack_end, module_mediator::return_value thread_id) {
	if (inner_check_function_signature(function_address, get_thread_local_structure()->initializer) != module_mediator::module_success) {
		delete[] get_thread_local_structure()->initializer;
		get_thread_local_structure()->initializer = nullptr;

		LOG_PROGRAM_ERROR(
			get_module_part(),
			"Function signature for function '" + get_exposed_function_name(function_address) +
			"' does not match."
		);

		return reinterpret_cast<std::uintptr_t>(nullptr);
	}
	
	module_mediator::arguments_string_type initializer = get_thread_local_structure()->initializer;
	if (initializer != nullptr) {
		module_mediator::arguments_array_type arguments =
			module_mediator::arguments_string_builder::convert_to_arguments_array(get_thread_local_structure()->initializer);

		std::uintptr_t result = inner_apply_initializer_on_thread_stack(thread_stack_memory, thread_stack_end, arguments);

		delete[] get_thread_local_structure()->initializer;
		get_thread_local_structure()->initializer = nullptr;

		return result;
	}

	return reinterpret_cast<std::uintptr_t>(thread_stack_memory);
}

