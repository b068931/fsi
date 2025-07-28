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
		helper.active_function.get_last_instruction().immediates.emplace_back(read_map.get_current_token()
        );

		helper.active_function.add_new_operand_to_last_instruction(
			read_map.get_current_token(),
			&helper.active_function.get_last_instruction().immediates.back(),
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
		structure_builder::immediate_type element_size{};
		source_file_token element_token = read_map.get_token_generator_additional_token();

		std::string variable_name{ helper.name_translations.translate_name(read_map.get_token_generator_name()) };
		structure_builder::function& current_function = helper.active_function.get_current_function();

		auto found_local = helper.active_function.find_local_variable_by_name(variable_name);
		if (found_local != current_function.locals.end()) {
			element_token = found_local->type;
		}
		else {
			auto found_argument = helper.active_function.find_argument_variable_by_name(variable_name);
			if (found_argument != current_function.arguments.end()) {
				element_token = found_argument->type;
			}
		}

		switch (element_token) {
		case source_file_token::one_byte_type_keyword:
			element_size = sizeof(module_mediator::one_byte);
			break;

		case source_file_token::two_bytes_type_keyword:
			element_size = sizeof(module_mediator::two_bytes);
			break;

		case source_file_token::four_bytes_type_keyword:
			element_size = sizeof(module_mediator::four_bytes);
			break;

		case source_file_token::eight_bytes_type_keyword:
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

		helper.active_function.get_last_instruction().immediates.back().imm_val = element_size;
	}
};

#endif