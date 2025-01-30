#ifndef IMMEDIATE_ARGUMENT_STATES_H
#define IMMEDIATE_ARGUMENT_STATES_H

#include "type_definitions.h"

class immediate_argument_type_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.current_function.get_last_instruction().immediates.push_back(structure_builder::imm_variable{ read_map.get_current_token() });
		helper.current_function.add_new_operand_to_last_instruction(
			read_map.get_current_token(),
			&helper.current_function.get_last_instruction().immediates.back(),
			false
		);
	}
};

class immediate_argument_value_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		try {
			helper.current_function.get_last_instruction().immediates.back().imm_val =
				helper.names_remapping.translate_name_to_integer<structure_builder::immediate_type>(read_map.get_token_generator_name());
		}
		catch (const std::exception& exc) {
			read_map.exit_with_error(exc.what());
		}
	}
};

#endif