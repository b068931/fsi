#include "dll_mediator.h"

void dll_mediator::dll_builder::configure_parse_map() {
	using state_settings =
		states_builder<data_file_token, context_key, std::vector<dll>, inter_states_parameters_structure, parameters_enumeration>::state_settings;

	using state_type =
		states_builder<data_file_token, context_key, std::vector<dll>, inter_states_parameters_structure, parameters_enumeration>::state_type;

	this->parameters.dll_part = this->mediator->get_dll_part();
	states_builder<data_file_token, context_key, std::vector<dll>, inter_states_parameters_structure, parameters_enumeration> builder{};

	state_settings& starting_state = builder.create_state<state_type>();
	state_settings& inside_header =
		builder.create_anonymous_state(
			[](std::vector<dll>& dlls, inter_states_parameters_structure& parameters, read_map_type& read_map) -> void {
				if (parameters.is_visible) {
					read_map.exit_with_error("You can not define the entire module as 'visible'");
				}

				switch (read_map.get_current_token()) {
				case data_file_token::name_and_public_name_separator:
					parameters.module_name = read_map.get_token_generator_name();
					break;

				case data_file_token::header_close:
					dlls.push_back(
						dll{
							std::move(parameters.module_name),
							read_map.get_token_generator_name(),
							parameters.dll_part
						}
					);
					break;
				}
			}
	);

	state_settings& base_state =
		builder.create_anonymous_state(
			[](std::vector<dll>& dlls, inter_states_parameters_structure& parameters, read_map_type& read_map) -> void {
				switch (read_map.get_current_token()) {
				case data_file_token::name_and_public_name_separator:
					parameters.function_name = 
						read_map.get_token_generator_name();

					break;

				case data_file_token::value_assign:
					parameters.function_name =
						read_map.get_token_generator_name();

					parameters.function_exported_name = parameters.function_name;
					break;

				case data_file_token::program_callable_function:
					if (parameters.is_visible) {
						read_map.exit_with_error("You can not define the same function as 'visible' two times in row.");
					}

					parameters.is_visible = true;
					break;
				}
			}
	);

	state_settings& function_with_exported_name =
		builder.create_anonymous_state(
			[](std::vector<dll>& dlls, inter_states_parameters_structure& parameters, read_map_type& read_map) -> void {
				if (read_map.is_token_generator_name_empty()) {
					read_map.exit_with_error();
				}
				else {
					parameters.function_exported_name =
						read_map.get_token_generator_name();
				}
			}
	);

	state_settings& inside_value =
		builder.create_anonymous_state(
			[](std::vector<dll>& dlls, inter_states_parameters_structure& parameters, read_map_type& read_map) -> void {
				std::string function_types_string = read_map.get_token_generator_name();

				arguments_string_type arguments_symbols = nullptr;
				if (function_types_string != "dynamic") {
					std::stringstream arguments_string{ function_types_string };
					arguments_symbols = new arguments_string_element[1]{ 0 };

					std::string argument;
					while (arguments_string.good()) { //won't execute if name string is empty
						arguments_string >> argument;

						if (!argument.empty()) {
							auto found_argument = std::find(parameters.arguments.begin(), parameters.arguments.end(), argument);
							if (found_argument != parameters.arguments.end()) {
								unsigned char previous_size = static_cast<unsigned char>(arguments_symbols[0]);

								arguments_string_type new_arguments_symbols = new arguments_string_element[static_cast<size_t>(previous_size) + 2];
								new_arguments_symbols[0] = previous_size + 1;

								std::memcpy(new_arguments_symbols + 1, arguments_symbols + 1, previous_size);
								new_arguments_symbols[static_cast<size_t>(new_arguments_symbols[0])] = static_cast<arguments_string_element>(found_argument - parameters.arguments.begin()); //difference_type for std::vector is signed integral type

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
	);

	state_settings& inside_comment = builder.create_state<state_type>();

	starting_state
		.set_as_starting_state()
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			data_file_token::header_open,
			state_action::change_top,
			inside_header
		);

	inside_header
		.detached_name(false)
		.set_error_message("Incorrectly defined header detected.")
		.set_handle_tokens(
			{
				data_file_token::header_close,
				data_file_token::name_and_public_name_separator
			}
		)
		.set_redirection_for_token(
			data_file_token::header_close,
			state_action::change_top,
			base_state
		);

	base_state
		.detached_name(false)
		.whitelist(false)
		.set_handle_tokens(
			{
				data_file_token::value_assign,
				data_file_token::program_callable_function,
				data_file_token::name_and_public_name_separator
			}
		)
		.set_redirection_for_token(
			data_file_token::header_open,
			state_action::change_top,
			inside_header
		)
		.set_redirection_for_token(
			data_file_token::value_assign,
			state_action::change_top,
			inside_value
		)
		.set_redirection_for_token(
			data_file_token::name_and_public_name_separator,
			state_action::change_top,
			function_with_exported_name
		)
		.set_redirection_for_token(
			data_file_token::comment,
			state_action::change_top,
			inside_comment
		);

	function_with_exported_name
		.detached_name(false)
		.set_error_message("You need to define the export name for your function.")
		.set_handle_tokens({ data_file_token::value_assign })
		.set_redirection_for_token(
			data_file_token::value_assign,
			state_action::change_top,
			inside_value
		);

	inside_value
		.detached_name(false)
		.set_error_message("Incorrectly set arguments types")
		.set_handle_tokens({ data_file_token::new_line, data_file_token::end_of_file })
		.set_redirection_for_token(
			data_file_token::new_line,
			state_action::change_top,
			base_state
		);

	inside_comment
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			data_file_token::new_line,
			state_action::change_top,
			base_state
		);

	builder.attach_states(&this->parse_map);
}