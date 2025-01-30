#ifndef PARSE_MAP_TYPE_DEFINITIONS_H
#define PARSE_MAP_TYPE_DEFINITIONS_H

#include <vector>
#include "module_mediator.h"

namespace module_mediator::parser {
	using state_settings = generic_parser::states_builder<
		components::engine_module_builder::file_tokens,
		components::engine_module_builder::context_keys,
		components::engine_module_builder::result_type,
		components::engine_module_builder::builder_parameters,
		components::engine_module_builder::dynamic_parameters_keys
	>::state_settings_type;

	using state_type = generic_parser::states_builder<
		components::engine_module_builder::file_tokens,
		components::engine_module_builder::context_keys,
		components::engine_module_builder::result_type,
		components::engine_module_builder::builder_parameters,
		components::engine_module_builder::dynamic_parameters_keys
	>::state_type;
}

#endif // !PARSE_MAP_TYPE_DEFINITIONS_H