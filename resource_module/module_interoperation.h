#ifndef RESOURCE_MODULE_MODULE_INTEROPERATION_H
#define RESOURCE_MODULE_MODULE_INTEROPERATION_H

#include "../module_mediator/module_part.h"
module_mediator::module_part* get_module_part();

class index_getter {
public:
	static std::size_t excm() {
		static std::size_t index = get_module_part()->find_module_index("excm");
		return index;
	}

	static std::size_t excm_on_container_creation() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "on_container_creation");
		return index;
	}

	static std::size_t excm_on_thread_creation() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "on_thread_creation");
		return index;
	}

	static std::size_t progload() {
		static std::size_t index = get_module_part()->find_module_index("progload");
		return index;
	}

	static std::size_t progload_free_program() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "free_program");
		return index;
	}
};

#endif