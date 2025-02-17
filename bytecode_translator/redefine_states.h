#ifndef REDEFINE_STATES_H
#define REDEFINE_STATES_H

#include <format>
#include "type_definitions.h"

class redefine_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string redefine_name = read_map.get_token_generator_name();
		if (helper.name_translations.has_remapping(redefine_name)) {
			read_map.exit_with_error(std::format("Name '{}' has been already redefined. You can use undefine to get rid of it.", redefine_name));
			return;
		}

		helper.name_translations.add(std::move(redefine_name), "");
	}
};

class redefine_value_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (read_map.get_token_generator_additional_token() != structure_builder::source_file_token::end_of_file) {
			auto names_stack = read_map
				.get_parameters_container()
				.retrieve_parameter<
					std::vector<std::pair<std::string, structure_builder::source_file_token>>*
				>(structure_builder::parameters_enumeration::names_stack);

			std::string new_keyword = std::move(helper.name_translations.back().first);
			names_stack->emplace_back(
				new_keyword,
				read_map.get_token_generator_additional_token()
			);

			helper.name_translations.remove(new_keyword);
		}
		else {
			helper.name_translations.back().second = read_map.get_token_generator_name();
		}
	}
};

#endif