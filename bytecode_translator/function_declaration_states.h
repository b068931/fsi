#ifndef FUNCTION_DECLARATION_STATES_H
#define FUNCTION_DECLARATION_STATES_H

#include "type_definitions.h"

class function_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		output_file_structure.functions.emplace_back(
            helper.get_id(),
				helper.name_translations.translate_name(read_map.get_token_generator_name())

        );
	}
};

class function_argument_type_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (read_map.get_current_token() == structure_builder::source_file_token::function_args_end) {
			if (!read_map.is_token_generator_name_empty()) {
				read_map.exit_with_error("Specify a name/type for your function argument.");
				return;
			}
		}
		else {
			output_file_structure.functions.back().arguments
				.emplace_back(
                    helper.get_id(),
						read_map.get_current_token()

                );
		}
	}
};

class function_argument_name_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		//this exists solely to make comments work
		if (!output_file_structure.functions.back().arguments.back().name.empty()) {
			return;
		}

		std::string generated_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
		if (!generated_name.empty()) {
			output_file_structure.functions.back().arguments.back().name =
				std::move(generated_name);
		}
		else {
			read_map.exit_with_error("Specify a name for your function argument.");
		}
	}
};

class declaration_or_definition_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		if (!read_map.is_token_generator_name_empty()) {
			read_map.exit_with_error();
		}

		helper.active_function.set_current_function(&output_file_structure.functions.back());
	}

};

#endif