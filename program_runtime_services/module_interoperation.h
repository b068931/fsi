#ifndef PROGRAM_RUNTIME_SERVICES_MODULE_INTEROPERATION_H
#define PROGRAM_RUNTIME_SERVICES_MODULE_INTEROPERATION_H

#include "../module_mediator/module_part.h"
#include "../module_mediator/fsi_types.h"
#include "../logger_module/logging.h"

#ifdef PROGRAMRUNTIMESERVICES_EXPORTS
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllexport)
#else
#define PROGRAMRUNTIMESERVICES_API extern "C" __declspec(dllimport) 
#endif

module_mediator::return_value get_current_thread_group_id();
std::pair<module_mediator::pointer, module_mediator::eight_bytes> decay_pointer(module_mediator::pointer);

module_mediator::module_part* get_module_part();
class index_getter {
public:
	static std::size_t progload() {
		static std::size_t index = get_module_part()->find_module_index("progload");
		return index;
	}

	static std::size_t progload_check_function_arguments() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "check_function_arguments");
		return index;
	}

	static std::size_t excm() {
		static std::size_t index = get_module_part()->find_module_index("excm");
		return index;
	}

	static std::size_t excm_get_current_thread_group_id() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_current_thread_group_id");
		return index;
	}

	static std::size_t excm_get_current_thread_id() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_current_thread_id");
		return index;
	}

	static std::size_t excm_switch_back() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "switch_back");
		return index;
	}

	static std::size_t excm_thread_terminate() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "thread_terminate");
		return index;
	}

	static std::size_t excm_nacreate_thread() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "nacreate_thread");
		return index;
	}

	static std::size_t excm_create_thread() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "create_thread");
		return index;
	}

	static std::size_t excm_get_thread_saved_variable() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "get_thread_saved_variable");
		return index;
	}

	static std::size_t excm_self_duplicate() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "self_duplicate");
		return index;
	}

	static std::size_t excm_self_priority() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "self_priority");
		return index;
	}

	static std::size_t excm_dynamic_call() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "dynamic_call");
		return index;
	}

	static std::size_t resm() {
		static std::size_t index = get_module_part()->find_module_index("resm");
		return index;
	}

	static std::size_t resm_allocate_program_memory() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "allocate_program_memory");
		return index;
	}

	static std::size_t resm_deallocate_program_memory() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "deallocate_program_memory");
		return index;
	}

	static std::size_t resm_get_jump_table() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_jump_table");
		return index;
	}

	static std::size_t resm_get_jump_table_size() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_jump_table_size");
		return index;
	}

	static std::size_t logger() {
		static std::size_t index = get_module_part()->find_module_index("logger");
		return index;
	}

	static std::size_t logger_program_info() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_info");
		return index;
	}

	static std::size_t logger_program_warning() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_warning");
		return index;
	}

	static std::size_t logger_program_error() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::logger(), "program_error");
		return index;
	}
};

#endif