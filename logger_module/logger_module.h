#ifndef LOGGER_MODULE_H
#define LOGGER_MODULE_H

#include "pch.h"
#include "../module_mediator/module_part.h"

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

#endif