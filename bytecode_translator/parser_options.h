#ifndef BYTECODE_TRANSLATOR_PARSER_OPTIONS_H
#define BYTECODE_TRANSLATOR_PARSER_OPTIONS_H

#include <map>
#include <vector>
#include <string>

#include "structure_builder.h"
#include "../generic_parser/token_generator.h"

class parser_options {
private:
	static const generic_parser::token_generator<
		structure_builder::source_file_token,
		structure_builder::context_key
	>::symbols_pair main_context;

	static const generic_parser::token_generator<
		structure_builder::source_file_token,
		structure_builder::context_key
	>::symbols_pair inside_string;

	static const generic_parser::token_generator<
		structure_builder::source_file_token,
		structure_builder::context_key
	>::symbols_pair inside_include;

public:
	static const std::vector<std::pair<std::string, structure_builder::source_file_token>> keywords;
	static const std::map<
		structure_builder::context_key,
		generic_parser::token_generator<
			structure_builder::source_file_token,
			structure_builder::context_key
		>::symbols_pair
	> contexts;
};

#endif