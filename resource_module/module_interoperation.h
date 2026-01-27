#ifndef RESOURCE_MODULE_MODULE_INTEROPERATION_H
#define RESOURCE_MODULE_MODULE_INTEROPERATION_H

#include "../module_mediator/module_part.h"

namespace interoperation {
	module_mediator::module_part* get_module_part();

	class index_getter {
	public:
		static std::size_t execution_module() {
			static std::size_t index = get_module_part()->find_module_index("excm");
			return index;
		}

		static std::size_t execution_module_on_container_creation() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::execution_module(), "on_container_creation");
			return index;
		}

		static std::size_t execution_module_on_thread_creation() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::execution_module(), "on_thread_creation");
			return index;
		}

		static std::size_t program_loader() {
			static std::size_t index = get_module_part()->find_module_index("progload");
			return index;
		}

		static std::size_t program_loader_free_program() {
			static std::size_t index = get_module_part()->find_function_index(index_getter::program_loader(), "free_program");
			return index;
		}
	};
}

#endif
