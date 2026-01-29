#ifndef	PROGRAM_LOADER_MODULE_INTEROPERATION_H
#define PROGRAM_LOADER_MODULE_INTEROPERATION_H

#ifdef PROGRAMLOADER_EXPORTS
#define COMPILERMODULE_API extern "C" __declspec(dllexport)
#else
#define COMPILERMODULE_API extern "C" __declspec(dllimport) 
#endif

#include "../module_mediator/module_part.h"

void* get_default_function_address();
namespace interoperation {
	module_mediator::module_part* get_module_part();

	class index_getter {
	public:
		static std::size_t resource_module() {
			static std::size_t index = get_module_part()->find_module_index("resm");
			return index;
		}

		static std::size_t resource_module_create_new_program_container() {
			static std::size_t index = get_module_part()->find_function_index(resource_module(), "create_new_program_container");
			return index;
		}
	};
}

#endif
