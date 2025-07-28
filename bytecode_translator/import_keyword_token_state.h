#ifndef IMPORT_KEYWORD_TOKEN_STATE_H
#define IMPORT_KEYWORD_TOKEN_STATE_H

#include "type_definitions.h"

class import_keyword_token_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (read_map.get_token_generator_additional_token() != source_file_token::import_keyword) {
			read_map.exit_with_error();
		}
	}
};

#endif