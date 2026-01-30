#ifndef FUNCTION_ADDRESS_ARGUMENT_STATE_H
#define FUNCTION_ADDRESS_ARGUMENT_STATE_H

#include "type_definitions.h"

class function_address_argument_state : public state_type {
public:
	void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.add_function_address_argument(output_file_structure, helper, read_map);
	}
};

#endif
