#ifndef FUNCTION_ADDRESS_ARGUMENT_STATE_H
#define FUNCTION_ADDRESS_ARGUMENT_STATE_H

#include "type_definitions.h"

class function_address_argument_state : public state_type {
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

		helper.add_function_address_argument(output_file_structure, helper, read_map);
	}
};

#endif
