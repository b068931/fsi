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
		bool just_left_comment = read_map
			.get_parameters_container()
			.retrieve_parameter<bool>(structure_builder::parameters_enumeration::has_just_left_comment);

		if (just_left_comment) {
			if (!read_map.is_token_generator_name_empty()) {
				read_map.exit_with_error();
			}

			return;
		}

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

		helper.current_function.get_last_instruction().strings.push_back({ &(found_string->second) });
		helper.current_function.add_new_operand_to_last_instruction(
			structure_builder::source_file_token::string_argument_keyword,
			&helper.current_function.get_last_instruction().strings.back(),
			false
		);
	}
};

#endif