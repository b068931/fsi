#include "parser_options.h"

const generic_parser::token_generator<
	module_mediator::parser::components::engine_module_builder::file_tokens,
	module_mediator::parser::components::engine_module_builder::context_keys
>::symbols_pair module_mediator::parser::parser_options::main_context{
	{
		{":", module_mediator::parser::components::engine_module_builder::file_tokens::name_and_public_name_separator},
		{"!", module_mediator::parser::components::engine_module_builder::file_tokens::program_callable_function},
		{"--", module_mediator::parser::components::engine_module_builder::file_tokens::comment},
		{"[", module_mediator::parser::components::engine_module_builder::file_tokens::header_open},
		{"]", module_mediator::parser::components::engine_module_builder::file_tokens::header_close},
		{"=", module_mediator::parser::components::engine_module_builder::file_tokens::value_assign},
		{"\n", module_mediator::parser::components::engine_module_builder::file_tokens::new_line},
		{"\r\n", module_mediator::parser::components::engine_module_builder::file_tokens::new_line}
	},
	{}
};

const std::vector<
	std::pair<
		std::string, 
		module_mediator::parser::components::engine_module_builder::file_tokens
	>
> module_mediator::parser::parser_options::keywords{};

const std::map<
	module_mediator::parser::components::engine_module_builder::context_keys,
	generic_parser::token_generator<
		module_mediator::parser::components::engine_module_builder::file_tokens,
		module_mediator::parser::components::engine_module_builder::context_keys
	>::symbols_pair
> module_mediator::parser::parser_options::contexts{
	{
		parser::components::engine_module_builder::context_keys::main_context,
		module_mediator::parser::parser_options::main_context
	}
};
