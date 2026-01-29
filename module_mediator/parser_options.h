#ifndef MODULE_MEDIATOR_PARSER_OPTIONS_H
#define MODULE_MEDIATOR_PARSER_OPTIONS_H

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "file_builder.h"
#include "../generic_parser/token_generator.h"

namespace module_mediator::parser {
	class parser_options {
        static const generic_parser::token_generator<
			components::engine_module_builder::file_tokens,
			components::engine_module_builder::context_keys
		>::symbols_pair main_context;

	public:
		static const std::vector<std::pair<std::string, components::engine_module_builder::file_tokens>> keywords;
		static const std::map<
			components::engine_module_builder::context_keys,
			generic_parser::token_generator<
				components::engine_module_builder::file_tokens,
				components::engine_module_builder::context_keys
			>::symbols_pair
		> contexts;
	};
}

#endif
