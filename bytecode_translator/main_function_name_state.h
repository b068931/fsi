#ifndef MAIN_FUNCTION_NAME_STATE_H
#define MAIN_FUNCTION_NAME_STATE_H

#include <algorithm>
#include <string>
#include <format>

#include "type_definitions.h"

class main_function_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string function_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
		auto found_function = std::ranges::find_if(
            output_file_structure.functions,
			[&function_name](const structure_builder::function& func) -> bool {
				return func.name == function_name;
			});

		if (found_function == output_file_structure.functions.end()) {
			read_map.exit_with_error(std::format("Function with name {} does not exist.", function_name));
			return;
		}

		output_file_structure.main_function = &*found_function;
	}
};

#endif