#ifndef DEFINE_STATE_H
#define DEFINE_STATE_H

#include "type_definitions.h"

class define_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.names_remapping.add(read_map.get_token_generator_name(), "");
	}
};

#endif