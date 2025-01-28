#ifndef PARSE_MAP_TYPE_DEFINITIONS_H
#define PARSE_MAP_TYPE_DEFINITIONS_H

#include <vector>
#include "dll_mediator.h"

using state_settings = generic_parser::states_builder<
	dll_builder::file_tokens, 
	dll_builder::context_keys, 
	dll_builder::result_type, 
	dll_builder::builder_parameters, 
	dll_builder::dynamic_parameters_keys
>::state_settings_type;

using state_type = generic_parser::states_builder<
	dll_builder::file_tokens, 
	dll_builder::context_keys, 
	dll_builder::result_type, 
	dll_builder::builder_parameters, 
	dll_builder::dynamic_parameters_keys
>::state_type;

#endif // !PARSE_MAP_TYPE_DEFINITIONS_H