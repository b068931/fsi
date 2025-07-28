#ifndef FSI_PARSER_MAIN_STATE_H
#define FSI_PARSER_MAIN_STATE_H

#include <algorithm>

#include "type_definitions.h"
#include "structure_builder.h"

class main_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		source_file_token token = read_map.get_current_token();
		if (token == source_file_token::function_body_start) {
			std::string name = helper.name_translations.translate_name(read_map.get_token_generator_name());
			auto found_function = std::ranges::find_if(output_file_structure.functions,
                                                       [&name](const structure_builder::function& function) -> bool {
                                                           return function.name == name;
                                                       }
            );

			if (found_function != output_file_structure.functions.end()) {
				helper.active_function.set_current_function(&*found_function);
			}
			else {
				read_map.exit_with_error("Function '" + name + "' was not declared. Declare a function before defining its body.");
			}
		}
		else if (token == source_file_token::expression_end && !read_map.is_token_generator_name_empty()) {
			read_map.exit_with_error();
		}
	}
};

#endif