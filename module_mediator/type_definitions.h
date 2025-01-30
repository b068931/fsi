#ifndef PARSE_MAP_TYPE_DEFINITIONS_H
#define PARSE_MAP_TYPE_DEFINITIONS_H

#include <vector>
#include "module_mediator.h"

using state_settings = generic_parser::states_builder<
	engine_module_builder::file_tokens, 
	engine_module_builder::context_keys, 
	engine_module_builder::result_type, 
	engine_module_builder::builder_parameters, 
	engine_module_builder::dynamic_parameters_keys
>::state_settings_type;

using state_type = generic_parser::states_builder<
	engine_module_builder::file_tokens, 
	engine_module_builder::context_keys, 
	engine_module_builder::result_type, 
	engine_module_builder::builder_parameters, 
	engine_module_builder::dynamic_parameters_keys
>::state_type;

#endif // !PARSE_MAP_TYPE_DEFINITIONS_H