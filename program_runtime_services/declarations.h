#ifndef PRTS_DECLARATIONS_H
#define PRTS_DECLARATIONS_H

#include "../module_mediator/module_part.h"
#include "../module_mediator/fsi_types.h"
#include "../console_and_debug/logging.h"

#ifdef PROGRAMRUNTIMESERVICES_EXPORTS
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllexport)
#else
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllimport) 
#endif

module_mediator::module_part* get_module_part();
module_mediator::return_value get_current_thread_group_id();

std::pair<module_mediator::pointer, module_mediator::eight_bytes> decay_pointer(module_mediator::pointer);

class index_getter {
public:
	static size_t progload() {
		static size_t index = get_module_part()->find_module_index("progload");
		return index;
	}

	static size_t progload_check_function_arguments() {
		static size_t index = get_module_part()->find_function_index(index_getter::progload(), "check_function_arguments");
		return index;
	}

	static size_t excm() {
		static size_t index = get_module_part()->find_module_index("excm");
		return index;
	}

	static size_t excm_get_current_thread_group_id() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_current_thread_group_id");
		return index;
	}

	static size_t excm_get_current_thread_id() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_current_thread_id");
		return index;
	}

	static size_t excm_switch_back() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "switch_back");
		return index;
	}

	static size_t excm_thread_terminate() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "thread_terminate");
		return index;
	}

	static size_t excm_nacreate_thread() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "nacreate_thread");
		return index;
	}

	static size_t excm_create_thread() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "create_thread");
		return index;
	}

	static size_t excm_get_thread_saved_variable() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_thread_saved_variable");
		return index;
	}

	static size_t excm_self_duplicate() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "self_duplicate");
		return index;
	}

	static size_t excm_self_priority() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "self_priority");
		return index;
	}

	static size_t excm_dynamic_call() {
		static size_t index = get_module_part()->find_function_index(index_getter::excm(), "dynamic_call");
		return index;
	}

	static size_t resm() {
		static size_t index = get_module_part()->find_module_index("resm");
		return index;
	}

	static size_t resm_allocate_program_memory() {
		static size_t index = get_module_part()->find_function_index(index_getter::resm(), "allocate_program_memory");
		return index;
	}

	static size_t resm_deallocate_program_memory() {
		static size_t index = get_module_part()->find_function_index(index_getter::resm(), "deallocate_program_memory");
		return index;
	}

	static size_t resm_get_jump_table() {
		static size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_jump_table");
		return index;
	}

	static size_t resm_get_jump_table_size() {
		static size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_jump_table_size");
		return index;
	}

	static size_t logger() {
		static size_t index = get_module_part()->find_module_index("logger");
		return index;
	}

	static size_t logger_program_info() {
		static size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_info");
		return index;
	}

	static size_t logger_program_warning() {
		static size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_warning");
		return index;
	}

	static size_t logger_program_error() {
		static size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_error");
		return index;
	}
};

#endif