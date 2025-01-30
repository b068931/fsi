#include "pch.h"
#include "functions.h"
#include "execution_module.h"
#include "thread_manager.h"
#include "program_state_manager.h"
#include "../program_loader/functions.h"
#include "../console_and_debug/logging.h"

module_mediator::module_part* part = nullptr;

const size_t thread_state_size = 144;
size_t thread_stack_size = 1000000;

char* program_control_functions_addresses = nullptr;

class index_getter { 
//i guess this class is thread safe https://stackoverflow.com/questions/8102125/is-local-static-variable-initialization-thread-safe-in-c11
public:
	static size_t progload() {
		static size_t index = ::part->find_module_index("progload");
		return index;
	}

	static size_t progload_load_program_to_memory() {
		static size_t index = ::part->find_function_index(index_getter::progload(), "load_program_to_memory");
		return index;
	}

	static size_t progload_check_function_arguments() {
		static size_t index = ::part->find_function_index(index_getter::progload(), "check_function_arguments");
		return index;
	}

	static size_t progload_get_function_name() {
		static size_t index = ::part->find_function_index(index_getter::progload(), "get_function_name");
		return index;
	}

	static size_t resm() {
		static size_t index = ::part->find_module_index("resm");
		return index;
	}

	static size_t resm_create_new_program_container() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "create_new_program_container");
		return index;
	}

	static size_t resm_create_new_thread() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "create_new_thread");
		return index;
	}

	static size_t resm_allocate_program_memory() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "allocate_program_memory");
		return index;
	}

	static size_t resm_allocate_thread_memory() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "allocate_thread_memory");
		return index;
	}

	static size_t resm_deallocate_program_container() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "deallocate_program_container");
		return index;
	}

	static size_t resm_deallocate_thread() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "deallocate_thread");
		return index;
	}

	static size_t resm_get_running_threads_count() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "get_running_threads_count");
		return index;
	}

	static size_t resm_get_program_container_id() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "get_program_container_id");
		return index;
	}

	static size_t resm_get_jump_table() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "get_jump_table");
		return index;
	}

	static size_t resm_get_jump_table_size() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "get_jump_table_size");
		return index;
	}

	static size_t resm_duplicate_container() {
		static size_t index = ::part->find_function_index(index_getter::resm(), "duplicate_container");
		return index;
	}
};
thread_manager manager;

module_mediator::return_value inner_deallocate_thread(module_mediator::return_value thread_id) {
	return module_mediator::fast_call<module_mediator::return_value>(
		::part,
		index_getter::resm(),
		index_getter::resm_deallocate_thread(),
		thread_id
	);
}
void inner_deallocate_program_container(module_mediator::return_value container_id) {
	module_mediator::fast_call<module_mediator::return_value>(
		::part,
		index_getter::resm(),
		index_getter::resm_deallocate_program_container(),
		container_id
	);
}
void inner_delete_running_thread() {
	thread_local_structure* thread_structure = get_thread_local_structure();
	module_mediator::return_value thread_group_id = thread_structure->currently_running_thread_information.thread_group_id;
	module_mediator::return_value thread_id = thread_structure->currently_running_thread_information.thread_id;

	bool is_delete_container = manager.delete_thread(thread_group_id, thread_id);

	log_info(::part, "Deallocating a thread. ID: " + std::to_string(thread_id));
	inner_deallocate_thread(thread_id);
	if (is_delete_container) {
		log_info(::part, "Deallocating a thread group. ID: " + std::to_string(thread_group_id));
		inner_deallocate_program_container(thread_group_id);
	}
}

void program_resume() {
	get_thread_local_structure()->currently_running_thread_information.state = scheduler::thread_states::running;
}
void thread_terminate() {
	log_program_info(::part, "Thread was terminated.");

	get_thread_local_structure()->currently_running_thread_information.put_back_structure = nullptr;
	inner_delete_running_thread();
}

void show_error(uint64_t error_code) {
	termination_codes termination_code = static_cast<termination_codes>(error_code);

	switch (termination_code) {
	case termination_codes::stack_overflow:
		log_program_error(::part, "Stack overflow. This can happen during function call, module function call, function prologue.");
		break;

	case termination_codes::nullptr_dereference:
		log_program_error(::part, "Uninitialized pointer dereference.");
		break;

	case termination_codes::pointer_out_of_bounds:
		log_program_error(::part, "Pointer index is out of bounds.");
		break;

	case termination_codes::undefined_function_call:
		log_program_error(::part, "Undefined function call. Called function has its own signature but it does not have a body.");
		break;

	case termination_codes::incorrect_saved_variable_type:
		log_program_error(::part, "Saved variable has different type.");
		break;

	case termination_codes::division_by_zero:
		log_program_error(::part, "Division by zero.");
		break;

	default:
		log_program_fatal(::part, "Unknown termination code. The process will be terminated with 'abort'.");
		std::abort();
	}

	log_program_error(::part, "Program execution error. Thread terminated.");
}
[[noreturn]] void inner_terminate(uint64_t error_code) {
	show_error(error_code);

	thread_terminate();
	load_execution_threadf(get_thread_local_structure()->execution_thread_state);
}

[[noreturn]] void inner_call_module_error(module_mediator::module_part::call_error error) {
	switch (error) {
	case module_mediator::module_part::call_error::function_is_not_visible:
		log_program_error(::part, "Called module function is not visible. Thread terminated.");
		break;

	case module_mediator::module_part::call_error::invalid_arguments_string:
		log_program_error(::part, "Incorrect arguments were used for the module function call. Thread terminated.");
		break;

	case module_mediator::module_part::call_error::unknown_index:
		log_program_error(::part, "Module function does not exist. Thread terminated.");
		break;
	}

	thread_terminate();
	load_execution_threadf(get_thread_local_structure()->execution_thread_state);
}
[[noreturn]] void inner_call_module(uint64_t module_id, uint64_t function_id, module_mediator::arguments_string_type args_string) {
	module_mediator::return_value action_code = ::part->call_module_visible_only(module_id, function_id, args_string, &inner_call_module_error);
	
	//all these functions are declared as [[noreturn]]
	switch (action_code)
	{
	case 0:
		program_resume();
		resume_program_executionf(get_thread_local_structure()->currently_running_thread_information.thread_state);

	case 1:
		load_execution_threadf(get_thread_local_structure()->execution_thread_state);

	case 2:
		log_program_info(::part, "Requested thread termination.");

		thread_terminate();
		load_execution_threadf(get_thread_local_structure()->execution_thread_state);

	default:
		log_program_fatal(::part, "Incorrect return code. Process will be killed with abort.");
		std::abort();
	}
}

module_mediator::return_value inner_self_duplicate(void* main_function, module_mediator::arguments_string_type initializer) {
	assert(get_thread_local_structure()->initializer == nullptr && "possible memory leak");
	get_thread_local_structure()->initializer = initializer;
	if (initializer != nullptr) { //you can not initialize a thread_group with pointer because this can lead to data race
		constexpr auto pointer_type_index = module_mediator::arguments_string_builder::get_type_index<module_mediator::pointer>;
		
		module_mediator::arguments_string_element arguments_count = initializer[0];
		module_mediator::arguments_string_type arguments_types = initializer + 1;
		for (module_mediator::arguments_string_element index = 0; index < arguments_count; ++index) {
			if (arguments_types[index] == static_cast<module_mediator::arguments_string_element>(pointer_type_index)) {
				delete[] initializer;
				get_thread_local_structure()->initializer = nullptr;

				log_program_error(::part, "You can not pass pointers as arguments for thread groups because this can easily lead to a data race.");
				return 1;
			}
		}
	}

	return module_mediator::fast_call<module_mediator::return_value, void*>(
		::part,
		index_getter::resm(),
		index_getter::resm_duplicate_container(),
		get_thread_local_structure()->currently_running_thread_information.thread_group_id,
		main_function
	);
}
module_mediator::return_value inner_get_container_running_threads_count(module_mediator::return_value container_id) {
	return module_mediator::fast_call<module_mediator::return_value>(
		::part,
		index_getter::resm(),
		index_getter::resm_get_running_threads_count(),
		container_id
	);
}

void inner_fill_in_reg_array_entry(uint64_t entry_index, char* memory, uint64_t value) {
	std::memcpy(memory + (sizeof(uint64_t) * entry_index), &value, sizeof(uint64_t));
}
module_mediator::return_value inner_create_thread(module_mediator::return_value thread_group_id, module_mediator::return_value priority, void* function_address) {
	thread_local_structure* thread_structure = get_thread_local_structure();
	log_program_info(::part, "Creating new thread.");

	thread_structure->program_function_address = function_address;
	thread_structure->priority = priority;

	return module_mediator::fast_call<module_mediator::return_value>(
		::part,
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
char* inner_allocate_thread_memory(module_mediator::return_value thread_id, uint64_t size) {
	return reinterpret_cast<char*>(
		module_mediator::fast_call<module_mediator::return_value, uint64_t>(
			::part,
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

	return module_mediator::fast_call<void*, uintptr_t>(
		::part,
		index_getter::progload(),
		index_getter::progload_check_function_arguments(),
		alleged_signature,
		reinterpret_cast<uintptr_t>(function_address)
	);
}
[[maybe_unused]] std::string get_exposed_function_name(void* function_address) {
	module_mediator::return_value functions_symbols_address = module_mediator::fast_call(
		::part,
		index_getter::progload(),
		index_getter::progload_get_function_name(),
		reinterpret_cast<uintptr_t>(function_address)
	);

	std::string function_name{ "[UNKNOWN_FUNCTION_NAME]" };
	char* function_symbols = reinterpret_cast<char*>(functions_symbols_address);
	if (function_symbols != nullptr) {
		function_name = function_symbols;
	}

	return function_name;
}
uintptr_t inner_apply_initializer_on_thread_stack(char* thread_stack_memory, char* thread_stack_end, const module_mediator::arguments_array_type& arguments) {
	for (module_mediator::arguments_array_type::const_reverse_iterator begin = arguments.rbegin(), end = arguments.rend(); begin != end; ++begin) {
		size_t type_size = module_mediator::arguments_string_builder::get_type_size_by_index(begin->first);
		if ((thread_stack_memory + type_size) >= thread_stack_end) {
			log_program_error(::part, "Not enough stack space to initialize thread stack.");
			reinterpret_cast<uintptr_t>(nullptr);
		}

		std::memcpy(
			thread_stack_memory,
			begin->second,
			type_size
		);

		thread_stack_memory += type_size;
	}

	return reinterpret_cast<uintptr_t>(thread_stack_memory);
}
uintptr_t inner_initialize_thread_stack(void* function_address, char* thread_stack_memory, char* thread_stack_end, module_mediator::return_value thread_id) {
	if (inner_check_function_signature(function_address, get_thread_local_structure()->initializer) != 0) {
		module_mediator::return_value container_id = inner_deallocate_thread(thread_id);
		if (inner_get_container_running_threads_count(container_id) == 0) {
			manager.forget_thread_group(container_id);
			inner_deallocate_program_container(container_id);
		}

		delete[] get_thread_local_structure()->initializer;
		get_thread_local_structure()->initializer = nullptr;

		log_program_error(
			::part,
			"Function signature for function '" + get_exposed_function_name(function_address) +
			"' does not match."
		);

		return reinterpret_cast<uintptr_t>(nullptr);
	}
	
	module_mediator::arguments_string_type initializer = get_thread_local_structure()->initializer;
	if (initializer != nullptr) {
		module_mediator::arguments_array_type arguments =
			module_mediator::arguments_string_builder::convert_to_arguments_array(get_thread_local_structure()->initializer);

		uintptr_t result = inner_apply_initializer_on_thread_stack(thread_stack_memory, thread_stack_end, arguments);

		delete[] get_thread_local_structure()->initializer;
		get_thread_local_structure()->initializer = nullptr;

		return result;
	}

	return reinterpret_cast<uintptr_t>(thread_stack_memory);
}

module_mediator::return_value on_thread_creation(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::return_value>(bundle);
	module_mediator::return_value container_id = std::get<0>(arguments);
	module_mediator::return_value thread_id = std::get<1>(arguments);

	thread_local_structure* thread_structure = get_thread_local_structure();

	char* thread_state_memory = inner_allocate_thread_memory(thread_id, thread_state_size);
	char* thread_stack_memory = inner_allocate_thread_memory(thread_id, thread_stack_size);
	
	char* thread_stack_end = thread_stack_memory + thread_stack_size - (sizeof(module_mediator::one_byte) + sizeof(uint64_t));

	//get program jump table from resm
	void* program_jump_table = reinterpret_cast<void*>(
		module_mediator::fast_call<module_mediator::return_value>(
			::part,
			index_getter::resm(),
			index_getter::resm_get_jump_table(),
			container_id
		)
	);

	//fill in thread_state - start
	inner_fill_in_reg_array_entry( //function address
		1, 
		thread_state_memory, 
		reinterpret_cast<uintptr_t>(thread_structure->program_function_address)
	);

	inner_fill_in_reg_array_entry( //jump table
		2,
		thread_state_memory,
		reinterpret_cast<uintptr_t>(program_jump_table)
	);

	inner_fill_in_reg_array_entry( //thread state
		3,
		thread_state_memory,
		reinterpret_cast<uintptr_t>(thread_state_memory)
	);

	uintptr_t result = inner_initialize_thread_stack(thread_structure->program_function_address, thread_stack_memory, thread_stack_end, thread_id);
	if (result == reinterpret_cast<uintptr_t>(nullptr)) {
		log_program_error(::part, "Thread stack initialization has failed.");
		return 1;
	}
	
	inner_fill_in_reg_array_entry( //stack current position
		4,
		thread_state_memory,
		result
	);

	inner_fill_in_reg_array_entry( //stack end
		5,
		thread_state_memory,
		reinterpret_cast<uintptr_t>(thread_stack_end) //account for the space that will be used to save the state of one variable between function calls
	);

	inner_fill_in_reg_array_entry( //program control functions
		6,
		thread_state_memory,
		reinterpret_cast<uintptr_t>(program_control_functions_addresses)
	);
	//fill in thread state - end

	manager.add_thread(
		container_id,
		thread_id,
		thread_structure->priority,
		thread_state_memory,
		program_jump_table
	);

	log_program_info(::part, "New thread has been successfully created.");
	return 0;
}
module_mediator::return_value on_container_creation(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*>(bundle);
	module_mediator::return_value container_id = std::get<0>(arguments);
	void* program_main = std::get<1>(arguments);

	manager.add_thread_group(container_id);
	log_program_info(::part, "New thread group has been successfully created.");

	return inner_create_thread(container_id, 0, program_main);
}

module_mediator::return_value self_duplicate(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<void*, void*, uint64_t>(bundle);

	module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[std::get<2>(arguments)]{};
	std::memcpy(copy, std::get<1>(arguments), std::get<2>(arguments));

	return inner_self_duplicate(
		std::get<0>(arguments),
		copy
	);
}
module_mediator::return_value self_priority(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.priority;
}
module_mediator::return_value get_thread_saved_variable(module_mediator::arguments_string_type) {
	char* thread_state = static_cast<char*>(get_thread_local_structure()->currently_running_thread_information.thread_state) + 40;
	module_mediator::pointer thread_stack_end{};

	std::memcpy(&thread_stack_end, thread_state, sizeof(module_mediator::pointer));
	return reinterpret_cast<uintptr_t>(thread_stack_end);
}
module_mediator::return_value dynamic_call(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<void*, void*>(bundle);
	void* function_address = std::get<0>(arguments);
	module_mediator::arguments_string_type function_arguments_string = static_cast<module_mediator::arguments_string_type>(std::get<1>(arguments));

	if (inner_check_function_signature(function_address, function_arguments_string) != 0) {
		log_program_error(
			::part, "A function signature for function '" + get_exposed_function_name(function_address) +
			"' does not match passed arguments. (dynamic call)"
		);

		return 1;
	}

	program_state_manager state_manager{ 
		static_cast<char*>(get_thread_local_structure()->currently_running_thread_information.thread_state) 
	};

	uintptr_t program_return_address = state_manager.get_return_address();
	module_mediator::arguments_array_type function_arguments = module_mediator::arguments_string_builder::convert_to_arguments_array(
		function_arguments_string
	);

	function_arguments.insert(
		function_arguments.cbegin(),
		{ static_cast<module_mediator::arguments_string_element>(module_mediator::arguments_string_builder::get_type_index<uintptr_t>), &program_return_address }
	);

	uintptr_t new_current_stack_position = inner_apply_initializer_on_thread_stack(
		reinterpret_cast<char*>(state_manager.get_current_stack_position()),
		reinterpret_cast<char*>(state_manager.get_stack_end()),
		function_arguments
	);

	if (new_current_stack_position == reinterpret_cast<uintptr_t>(nullptr)) {
		log_program_error(::part, "The thread stack initialization has failed. (dynamic call)");
		return 1;
	}

	state_manager.set_current_stack_position(
		new_current_stack_position - module_mediator::arguments_string_builder::get_type_size_by_index(module_mediator::arguments_string_builder::get_type_index<uintptr_t>)
	);
	
	state_manager.set_return_address(
		reinterpret_cast<uintptr_t>(static_cast<char*>(function_address) + function_save_return_address_size)
	);

	return 0;
}

module_mediator::return_value self_block(module_mediator::arguments_string_type) {
	log_program_info(::part, "Was blocked.");
	manager.block(get_thread_local_structure()->currently_running_thread_information.thread_id);

	return 0;
}
module_mediator::return_value get_current_thread_id(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.thread_id;
}
module_mediator::return_value get_current_thread_group_id(module_mediator::arguments_string_type) {
	return get_thread_local_structure()->currently_running_thread_information.thread_group_id;
}
module_mediator::return_value make_runnable(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value>(bundle);

	module_mediator::return_value thread_id = std::get<0>(arguments);
	log_program_info(::part, "Made a thread with id '" + std::to_string(thread_id) + "' runnable again.");

	manager.make_runnable(thread_id);
	return 0;
}
module_mediator::return_value start(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value>(bundle);
	uint16_t thread_count = static_cast<uint16_t>(std::get<0>(arguments));

	log_info(::part, "Creating execution daemons. Count: " + std::to_string(thread_count));
	manager.startup(thread_count);

	return 0;
}
module_mediator::return_value create_thread(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*, uint64_t>(bundle);

	module_mediator::arguments_string_type copy = new module_mediator::arguments_string_element[std::get<3>(arguments)]{};
	std::memcpy(copy, std::get<2>(arguments), std::get<3>(arguments));

	return inner_create_thread_with_initializer(
		std::get<0>(arguments),
		std::get<1>(arguments),
		copy
	);

	return 0;
}
module_mediator::return_value run_program(module_mediator::arguments_string_type bundle) {
	//excm(run_program)->prog_load(load_program_to_memory)->resm(create_new_prog_container)->excm(on_container_creation)
	auto arguments = module_mediator::arguments_string_builder::unpack<void*>(bundle);
	log_program_info(::part, "Starting new program.");

	module_mediator::fast_call<void*>(
		::part,
		index_getter::progload(),
		index_getter::progload_load_program_to_memory(),
		std::get<0>(arguments)
	);

	return 0;
}

[[noreturn]] void end_program() {
	thread_terminate();
	load_execution_threadf(get_thread_local_structure()->execution_thread_state);
}
void initialize_m(module_mediator::module_part* part) {
	::part = part;
	::program_control_functions_addresses = new char[4 * sizeof(uint64_t)] {};
	
	inner_fill_in_reg_array_entry(
		0,
		::program_control_functions_addresses,
		reinterpret_cast<uintptr_t>(&special_call_modulef)
	);

	inner_fill_in_reg_array_entry(
		1,
		::program_control_functions_addresses,
		reinterpret_cast<uintptr_t>(&inner_call_module)
	);

	inner_fill_in_reg_array_entry(
		2,
		::program_control_functions_addresses,
		reinterpret_cast<uintptr_t>(&inner_terminate)
	);

	inner_fill_in_reg_array_entry(
		3,
		::program_control_functions_addresses,
		reinterpret_cast<uintptr_t>(&end_program)
	);
}