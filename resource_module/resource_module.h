#ifndef RESOURCE_MODULE
#define RESOURCE_MODULE

#include "pch.h"

#ifdef RESOURCEMODULE_EXPORTS
#define RESOURCEMODULE_API extern "C" __declspec(dllexport)
#else
#define RESOURCEMODULE_API extern "C" __declspec(dllimport) 
#endif

class id_generator {
public:
	using id_type = module_mediator::return_value;

private:
	static std::stack<id_type> free_ids;
	static std::mutex lock;
	static id_type current_id;

public:
	static id_type get_id() {
		std::lock_guard lock{ id_generator::lock };
		if (!free_ids.empty()) {
			id_type id = free_ids.top();
			free_ids.pop();

			return id;
		}

		return current_id++;
	}
	static void free_id(id_type id) {
		std::lock_guard lock{ id_generator::lock };
		free_ids.push(id);
		assert(id != 0);
	}
};

RESOURCEMODULE_API module_mediator::return_value add_container_on_destroy(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value add_thread_on_destroy(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value duplicate_container(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value create_new_program_container(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value create_new_thread(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value allocate_program_memory(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value allocate_thread_memory(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value deallocate_program_memory(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value deallocate_thread_memory(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value deallocate_program_container(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value deallocate_thread(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value get_running_threads_count(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value get_program_container_id(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API module_mediator::return_value get_jump_table(module_mediator::arguments_string_type bundle);
RESOURCEMODULE_API module_mediator::return_value get_jump_table_size(module_mediator::arguments_string_type bundle);

RESOURCEMODULE_API void initialize_m(module_mediator::dll_part*);

#endif // !RESOURCE_MODULE