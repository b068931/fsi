#ifndef EXECUTION_CONTROL_FUNCTIONS_H
#define EXECUTION_CONTROL_FUNCTIONS_H

#include "pch.h"
#include "thread_local_structure.h"
#include "../module_mediator/module_part.h"

char* get_program_control_functions_addresses();
extern thread_local_structure* get_thread_local_structure();

namespace interoperation {
	module_mediator::module_part* get_module_part();

	class index_getter {
		// I guess this class is thread safe:
		// https://stackoverflow.com/questions/8102125/is-local-static-variable-initialization-thread-safe-in-c11

	public:
		static std::size_t program_loader() {
			static std::size_t index = get_module_part()->find_module_index("progload");
			return index;
		}

		static std::size_t program_loader_load_program_to_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::program_loader(), "load_program_to_memory");
			return index;
		}

		static std::size_t program_loader_check_function_arguments() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::program_loader(), "check_function_arguments");
			return index;
		}

		static std::size_t program_loader_get_function_name() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::program_loader(), "get_function_name");
			return index;
		}

		static std::size_t resource_module() {
			static std::size_t index = get_module_part()->find_module_index("resm");
			return index;
		}

		static std::size_t resource_module_create_new_program_container() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "create_new_program_container");
			return index;
		}

		static std::size_t resource_module_create_new_thread() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "create_new_thread");
			return index;
		}

		static std::size_t resource_module_allocate_program_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "allocate_program_memory");
			return index;
		}

		static std::size_t resource_module_allocate_thread_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "allocate_thread_memory");
			return index;
		}

		static std::size_t resource_module_deallocate_program_container() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "deallocate_program_container");
			return index;
		}

		static std::size_t resource_module_deallocate_thread() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "deallocate_thread");
			return index;
		}

		static std::size_t resource_module_get_running_threads_count() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "get_running_threads_count");
			return index;
		}

		static std::size_t resource_module_get_program_container_id() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "get_program_container_id");
			return index;
		}

		static std::size_t resource_module_get_jump_table() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "get_jump_table");
			return index;
		}

		static std::size_t resource_module_get_jump_table_size() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "get_jump_table_size");
			return index;
		}

		static std::size_t resource_module_duplicate_container() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "duplicate_container");
			return index;
		}

		static std::size_t resource_module_deallocate_thread_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "deallocate_thread_memory");
			return index;
		}

		static std::size_t resource_module_deallocate_program_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "deallocate_program_memory");
			return index;
		}

		static std::size_t resource_module_verify_thread_memory() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::resource_module(), "verify_thread_memory");
			return index;
        }
	};
}

#endif