#ifndef IMMEDIATE_ARGUMENT_STATES_H
#define IMMEDIATE_ARGUMENT_STATES_H

#include "type_definitions.h"

class immediate_argument_type_state : public state_type {
public:
	void handle_token(
		structure_builder::file&,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.active_function.get_last_instruction().immediates.emplace_back(read_map.get_current_token());
		helper.active_function.add_new_operand_to_last_instruction(
			read_map.get_current_token(),
			&helper.active_function.get_last_instruction().immediates.back(),
			false
		);
	}
};

class immediate_argument_value_state : public state_type {
public:
	void handle_token(
		structure_builder::file&,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		try {
			helper.active_function.get_last_instruction().immediates.back().imm_val =
				helper.name_translations.translate_name_to_integer<structure_builder::immediate_type>(read_map.get_token_generator_name());
		}
		catch (const std::exception& exc) {
			read_map.exit_with_error(exc.what());
		}
	}
};

#endif
