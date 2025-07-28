#ifndef STRING_ARGUMENT_STATE_H
#define STRING_ARGUMENT_STATE_H

#include "type_definitions.h"

class string_argument_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string string_name = read_map.get_token_generator_name();
		if (string_name.empty()) {
			read_map.exit_with_error("Expected the name of the string, got empty string instead.");
			return;
		}

		auto found_string = output_file_structure.program_strings.find(string_name);
		if (found_string == output_file_structure.program_strings.end()) {
			read_map.exit_with_error("String with name '" + string_name + "' does not exist.");
			return;
		}

		helper.active_function.get_last_instruction().strings.emplace_back(&found_string->second);
		helper.active_function.add_new_operand_to_last_instruction(
			source_file_token::string_argument_keyword,
			&helper.active_function.get_last_instruction().strings.back(),
			false
		);
	}
};

#endif