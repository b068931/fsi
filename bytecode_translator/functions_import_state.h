#ifndef FUNCTION_IMPORT_STATE_H
#define FUNCTION_IMPORT_STATE_H

#include "type_definitions.h"
#include <algorithm>
#include <format>

class functions_import_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string module_function_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
		if (module_function_name.empty()) {
			if ((read_map.get_current_token() != structure_builder::source_file_token::import_end)
				&& (read_map.get_current_token() != structure_builder::source_file_token::comment_start)) {
				read_map.exit_with_error("Name of the module function was expected, got empty string instead.");
			}

			return;
		}

		auto found_module_function = std::ranges::find_if(helper.current_module->functions_names,
                                                          [&module_function_name](const structure_builder::module_function& fnc) -> bool {
                                                              return fnc.name == module_function_name;
                                                          });

		if (found_module_function == helper.current_module->functions_names.end()) {
			helper.current_module->functions_names
				.emplace_back(helper.get_id(), std::move(module_function_name));
		}
	}
};

#endif