#ifndef SOURCE_PARSER_TYPE_DEFINITIONS_H
#define SOURCE_PARSER_TYPE_DEFINITIONS_H

#include "structure_builder.h"
#include "../generic_parser/read_map.h"

using states_builder_type = 
	generic_parser::states_builder<
		source_file_token, 
		structure_builder::context_key, 
		structure_builder::file, 
		structure_builder::builder_parameters, 
		structure_builder::parameters_enumeration
	>;

using state_type = states_builder_type::state_type;
using state_settings = states_builder_type::state_settings_type;

#endif