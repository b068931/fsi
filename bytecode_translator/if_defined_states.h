#ifndef IF_DEFINED_STATES_H
#define IF_DEFINED_STATES_H

#include "type_definitions.h"

class if_defined_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		read_map
			.get_parameters_container()
			.assign_parameter(
				structure_builder::parameters_enumeration::ifdef_ifndef_pop_check,
				helper.name_translations.has_remapping(read_map.get_token_generator_name())
			);
	}
};

class if_not_defined_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		read_map
			.get_parameters_container()
			.assign_parameter(
				structure_builder::parameters_enumeration::ifdef_ifndef_pop_check,
				!helper.name_translations.has_remapping(read_map.get_token_generator_name())
			);
	}
};

class ignore_all_until_endif_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		read_map
			.get_parameters_container()
			.assign_parameter(structure_builder::parameters_enumeration::ifdef_ifndef_pop_check, false);
	}
};

#endif