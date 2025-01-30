#ifndef CONSOLE_AND_DEBUG_H
#define CONSOLE_AND_DEBUG_H

#include "pch.h"

#ifdef CONSOLEANDDEBUG_EXPORTS
#define CONSOLEANDDEBUG_API extern "C" __declspec(dllexport)
#else
#define CONSOLEANDDEBUG_API extern "C" __declspec(dllimport)
#endif

CONSOLEANDDEBUG_API module_mediator::return_value info(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value warning(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value error(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value fatal(module_mediator::arguments_string_type bundle);

CONSOLEANDDEBUG_API module_mediator::return_value program_info(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value program_warning(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value program_error(module_mediator::arguments_string_type bundle);
CONSOLEANDDEBUG_API module_mediator::return_value program_fatal(module_mediator::arguments_string_type bundle);

CONSOLEANDDEBUG_API void initialize_m(module_mediator::module_part*);

module_mediator::module_part* part;
class index_getter {
public:
	static std::size_t excm() {
		static std::size_t index = ::part->find_module_index("excm");
		return index;
	}

	static std::size_t excm_get_current_thread_id() {
		static std::size_t index = ::part->find_function_index(index_getter::excm(), "get_current_thread_id");
		return index;
	}

	static std::size_t excm_get_current_thread_group_id() {
		static std::size_t index = ::part->find_function_index(index_getter::excm(), "get_current_thread_group_id");
		return index;
	}
};

#endif