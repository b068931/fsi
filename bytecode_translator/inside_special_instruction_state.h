#ifndef INSIDE_SPECIAL_INSTRUCTION_STATE_H
#define INSIDE_SPECIAL_INSTRUCTION_STATE_H

#include "type_definitions.h"

class inside_special_instruction_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (read_map.get_current_token() == structure_builder::source_file_token::include_keyword) {
			read_map.switch_context(structure_builder::context_key::inside_include);
		}
	}
};

#endif