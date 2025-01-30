#ifndef REDEFINE_STATES_H
#define REDEFINE_STATES_H

#include "type_definitions.h"

class redefine_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.names_remapping.add(read_map.get_token_generator_name(), "");
	}
};

class redefine_value_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.names_remapping.back().second = read_map.get_token_generator_name();
	}
};

#endif