#ifndef SIZEOF_ARGUMENT_STATES_H
#define SIZEOF_ARGUMENT_STATES_H

#include "type_definitions.h"
#include "../module_mediator/fsi_types.h"

class sizeof_argument_type_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.current_function.get_last_instruction().immediates.push_back(
			structure_builder::imm_variable{ read_map.get_current_token() }
		);

		helper.current_function.add_new_operand_to_last_instruction(
			read_map.get_current_token(),
			&helper.current_function.get_last_instruction().immediates.back(),
			false
		);
	}
};

class sizeof_argument_value_state : public state_type {
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

		structure_builder::immediate_type element_size{};
		structure_builder::source_file_token element_token = read_map.get_token_generator_additional_token();

		std::string variable_name{ helper.names_remapping.translate_name(read_map.get_token_generator_name()) };
		structure_builder::function& current_function = helper.current_function.get_current_function();

		auto found_local = helper.current_function.find_local_variable_by_name(variable_name);
		if (found_local != current_function.locals.end()) {
			element_token = found_local->type;
		}
		else {
			auto found_argument = helper.current_function.find_argument_variable_by_name(variable_name);
			if (found_argument != current_function.arguments.end()) {
				element_token = found_argument->type;
			}
		}

		switch (element_token) {
		case structure_builder::source_file_token::one_byte_type_keyword:
			element_size = sizeof(module_mediator::one_byte);
			break;

		case structure_builder::source_file_token::two_bytes_type_keyword:
			element_size = sizeof(module_mediator::two_bytes);
			break;

		case structure_builder::source_file_token::four_bytes_type_keyword:
			element_size = sizeof(module_mediator::four_bytes);
			break;

		case structure_builder::source_file_token::eight_bytes_type_keyword:
			element_size = sizeof(module_mediator::eight_bytes);
			break;

		default:
			auto found_string = output_file_structure.program_strings.find(variable_name);
			if (found_string != output_file_structure.program_strings.end()) {
				element_size = found_string->second.value.size() + 1;
				break;
			}

			read_map.exit_with_error();
			return;
		}

		helper.current_function.get_last_instruction().immediates.back().imm_val = element_size;
	}
};

#endif