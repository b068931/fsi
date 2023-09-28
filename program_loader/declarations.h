#ifndef	DECLARATIONS_H
#define DECLARATIONS_H

#ifdef PROGRAMLOADER_EXPORTS
#define COMPILERMODULE_API extern "C" __declspec(dllexport)
#else
#define COMPILERMODULE_API extern "C" __declspec(dllimport) 
#endif

#include "../dll_mediator/dll_part.h"

dll_part* get_dll_part();
void* get_default_function_address();

class index_getter {
public:
	static size_t resm() {
		static size_t index = get_dll_part()->find_dll_index("resm");
		return index;
	}

	static size_t resm_create_new_program_container() {
		static size_t index = get_dll_part()->find_function_index(index_getter::resm(), "create_new_program_container");
		return index;
	}
};

#endif