#ifndef PROGRAM_STRINGS_STATES_H
#define PROGRAM_STRINGS_STATES_H

#include "type_definitions.h"

class string_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		auto[iterator, is_inserted] = output_file_structure.program_strings.insert(
			{ read_map.get_token_generator_name(), structure_builder::string{ helper.get_id() } }
		);

		if (!is_inserted) {
			read_map.exit_with_error("String with key '" + iterator->first + "' already exists.");
			return;
		}

		helper.current_string = std::move(iterator);
		read_map.switch_context(structure_builder::context_key::inside_string);
	}
};

class string_value_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file&,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		helper.current_string->second.value = read_map.get_token_generator_name();
		read_map.switch_context(structure_builder::context_key::main_context);
	}
};

#endif
