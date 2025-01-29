#ifndef INSIDE_VALUE_STATE_H
#define INSIDE_VALUE_STATE_H

#include "type_definitions.h"

class inside_value_state : public state_type {
public:
	virtual void handle_token(
		dll_builder::result_type& dlls,
		dll_builder::builder_parameters& parameters,
		dll_builder::read_map_type& read_map
	) override {
		std::string function_types_string = read_map.get_token_generator_name();

		module_mediator::arguments_string_type arguments_symbols = nullptr;
		if (function_types_string != "dynamic") {
			std::stringstream arguments_string{ function_types_string };
			arguments_symbols = new module_mediator::arguments_string_element[1]{ 0 };

			std::string argument;
			while (arguments_string.good()) { //won't execute if name string is empty
				arguments_string >> argument;

				if (!argument.empty()) {
					auto found_argument = std::find(parameters.arguments.begin(), parameters.arguments.end(), argument);
					if (found_argument != parameters.arguments.end()) {
						unsigned char previous_size = static_cast<unsigned char>(arguments_symbols[0]);

						module_mediator::arguments_string_type new_arguments_symbols = new module_mediator::arguments_string_element[static_cast<size_t>(previous_size) + 2];
						new_arguments_symbols[0] = previous_size + 1;

						std::memcpy(new_arguments_symbols + 1, arguments_symbols + 1, previous_size);
						new_arguments_symbols[static_cast<size_t>(new_arguments_symbols[0])] = static_cast<module_mediator::arguments_string_element>(found_argument - parameters.arguments.begin()); //difference_type for std::vector is signed integral type

						delete[] arguments_symbols;
						arguments_symbols = new_arguments_symbols;
					}
				}
			}
		}

		bool result = dlls.back().add_function(
				parameters.function_name,
				std::move(parameters.function_exported_name),
				arguments_symbols,
				parameters.is_visible
		);

		if (!result) {
			read_map.exit_with_error(
				"Unable to load function '" + 
				parameters.function_name + "' from module '" + 
				dlls.back().get_name() + "'"
			);
		}

		parameters.is_visible = false;
	}
};

#endif