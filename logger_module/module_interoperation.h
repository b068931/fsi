#ifndef LOGGER_MODULE_INTEROPERATION_H
#define LOGGER_MODULE_INTEROPERATION_H

#include "pch.h"
#include "../module_mediator/module_part.h"

module_mediator::module_part* get_module_part();
class index_getter {
public:
	static std::size_t excm() {
		static std::size_t index = get_module_part()->find_module_index("excm");
		return index;
	}

	static std::size_t excm_get_current_thread_id() {
		static std::size_t index = get_module_part()->find_function_index(excm(), "get_current_thread_id");
		return index;
	}

	static std::size_t excm_get_current_thread_group_id() {
		static std::size_t index = get_module_part()->find_function_index(excm(), "get_current_thread_group_id");
		return index;
	}
};

#endif
