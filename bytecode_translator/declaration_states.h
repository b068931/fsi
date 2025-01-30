#ifndef DECLARATION_STATES_H
#define DECLARATION_STATES_H

#include "type_definitions.h"

class declare_type_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (helper.current_function.is_current_function_present()) {
			helper.current_function.get_current_function().locals
				.push_back(
					structure_builder::regular_variable{
						helper.get_id(),
						read_map.get_current_token()
					}
			);
		}
		else {
			read_map.exit_with_error();
		}
	}
};

class declare_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.current_function.get_current_function().locals.back().name =
			helper.names_remapping.translate_name(read_map.get_token_generator_name());
	};
};

#endif