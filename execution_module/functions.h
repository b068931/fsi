#ifndef EXECUTION_CONTROL_FUNCTIONS_H
#define EXECUTION_CONTROL_FUNCTIONS_H

#include "pch.h"
#include "thread_local_structure.h"
#include "../module_mediator/module_part.h"

extern thread_local_structure* get_thread_local_structure();
module_mediator::module_part* get_module_part();
char* get_program_control_functions_addresses();

extern "C" [[noreturn]] void load_execution_threadf(void*);
extern "C" void load_programf(void*, void*, std::size_t is_startup);
extern "C" module_mediator::return_value special_call_modulef();
extern "C" [[noreturn]] void resume_program_executionf(void*);

class index_getter { 
//i guess this class is thread safe https://stackoverflow.com/questions/8102125/is-local-static-variable-initialization-thread-safe-in-c11
public:
	static std::size_t progload() {
		static std::size_t index = get_module_part()->find_module_index("progload");
		return index;
	}

	static std::size_t progload_load_program_to_memory() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "load_program_to_memory");
		return index;
	}

	static std::size_t progload_check_function_arguments() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "check_function_arguments");
		return index;
	}

	static std::size_t progload_get_function_name() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "get_function_name");
		return index;
	}

	static std::size_t resm() {
		static std::size_t index = get_module_part()->find_module_index("resm");
		return index;
	}

	static std::size_t resm_create_new_program_container() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "create_new_program_container");
		return index;
	}

	static std::size_t resm_create_new_thread() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "create_new_thread");
		return index;
	}

	static std::size_t resm_allocate_program_memory() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "allocate_program_memory");
		return index;
	}

	static std::size_t resm_allocate_thread_memory() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "allocate_thread_memory");
		return index;
	}

	static std::size_t resm_deallocate_program_container() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "deallocate_program_container");
		return index;
	}

	static std::size_t resm_deallocate_thread() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "deallocate_thread");
		return index;
	}

	static std::size_t resm_get_running_threads_count() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_running_threads_count");
		return index;
	}

	static std::size_t resm_get_program_container_id() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "get_program_container_id");
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

	static std::size_t resm_duplicate_container() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::resm(), "duplicate_container");
		return index;
	}
};

#endif