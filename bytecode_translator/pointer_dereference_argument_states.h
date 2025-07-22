#ifndef POINTER_DEREFERENCE_ARGUMENT_STATES_H
#define POINTER_DEREFERENCE_ARGUMENT_STATES_H

#include "type_definitions.h"

class pointer_dereference_argument_type_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.active_function.get_last_instruction().dereferences.emplace_back();
		helper.active_function.add_new_operand_to_last_instruction(
			read_map.get_current_token(),
			&helper.active_function.get_last_instruction().dereferences.back(),
			false
		);
	}
};

class pointer_dereference_argument_pointer_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.active_function.map_operand_with_variable(
			helper.name_translations.translate_name(read_map.get_token_generator_name()),
			&helper.active_function.get_last_instruction().dereferences.back().pointer_variable,
			read_map
		);
	}
};

class pointer_dereference_argument_dereference_variables_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string dereference_variable_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
		if (dereference_variable_name.empty()) {
			if (
				(read_map.get_current_token() != structure_builder::source_file_token::dereference_end)
				&& (read_map.get_current_token() != structure_builder::source_file_token::comment_start)) {
				read_map.exit_with_error("Expected the name of the dereference variable, got empty string instead.");
			}

			return;
		}

		helper.active_function.get_last_instruction().dereferences.back().derefernce_indexes.push_back(nullptr);
		helper.active_function.map_operand_with_variable(
			std::move(dereference_variable_name),
			&helper.active_function.get_last_instruction().dereferences.back().derefernce_indexes.back(),
			read_map
		);
	}
};

#endif