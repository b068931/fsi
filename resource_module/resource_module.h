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
	using id_type = return_value;

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

RESOURCEMODULE_API return_value add_container_on_destroy(arguments_string_type bundle);
RESOURCEMODULE_API return_value add_thread_on_destroy(arguments_string_type bundle);

RESOURCEMODULE_API return_value duplicate_container(arguments_string_type bundle);

RESOURCEMODULE_API return_value create_new_program_container(arguments_string_type bundle);
RESOURCEMODULE_API return_value create_new_thread(arguments_string_type bundle);

RESOURCEMODULE_API return_value allocate_program_memory(arguments_string_type bundle);
RESOURCEMODULE_API return_value allocate_thread_memory(arguments_string_type bundle);

RESOURCEMODULE_API return_value deallocate_program_memory(arguments_string_type bundle);
RESOURCEMODULE_API return_value deallocate_thread_memory(arguments_string_type bundle);

RESOURCEMODULE_API return_value deallocate_program_container(arguments_string_type bundle);
RESOURCEMODULE_API return_value deallocate_thread(arguments_string_type bundle);

RESOURCEMODULE_API return_value get_running_threads_count(arguments_string_type bundle);
RESOURCEMODULE_API return_value get_program_container_id(arguments_string_type bundle);

RESOURCEMODULE_API return_value get_jump_table(arguments_string_type bundle);
RESOURCEMODULE_API return_value get_jump_table_size(arguments_string_type bundle);

RESOURCEMODULE_API void initialize_m(dll_part*);

#endif // !RESOURCE_MODULE