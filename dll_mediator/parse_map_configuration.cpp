#include "dll_mediator.h"
#include "inside_header_state.h"
#include "main_state.h"
#include "function_with_exported_name_state.h"
#include "inside_value_state.h"

void dll_builder::configure_parse_map() {
	using state_type =
		generic_parser::states_builder<file_tokens, context_keys, result_type, builder_parameters, dynamic_parameters_keys>::state_type;

	this->parameters.dll_part = this->mediator->get_dll_part();
	generic_parser::states_builder<file_tokens, context_keys, result_type, builder_parameters, dynamic_parameters_keys> builder{};

	state_settings& starting_state = builder.create_state<state_type>();
	state_settings& base_state = builder.create_state<main_state>();
	state_settings& inside_header = builder.create_state<inside_header_state>();
	state_settings& function_with_exported_name = builder.create_state<function_with_exported_name_state>();
	state_settings& inside_value = builder.create_state<inside_value_state>();
	state_settings& inside_comment = builder.create_state<state_type>();

	starting_state
		.set_as_starting_state()
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			file_tokens::header_open,
			generic_parser::state_action::change_top,
			inside_header
		);

	inside_header
		.detached_name(false)
		.set_error_message("Incorrectly defined header detected.")
		.set_handle_tokens(
			{
				file_tokens::header_close,
				file_tokens::name_and_public_name_separator
			}
		)
		.set_redirection_for_token(
			file_tokens::header_close,
			generic_parser::state_action::change_top,
			base_state
		);

	base_state
		.detached_name(false)
		.whitelist(false)
		.set_handle_tokens(
			{
				file_tokens::value_assign,
				file_tokens::program_callable_function,
				file_tokens::name_and_public_name_separator
			}
		)
		.set_redirection_for_token(
			file_tokens::header_open,
			generic_parser::state_action::change_top,
			inside_header
		)
		.set_redirection_for_token(
			file_tokens::value_assign,
			generic_parser::state_action::change_top,
			inside_value
		)
		.set_redirection_for_token(
			file_tokens::name_and_public_name_separator,
			generic_parser::state_action::change_top,
			function_with_exported_name
		)
		.set_redirection_for_token(
			file_tokens::comment,
			generic_parser::state_action::change_top,
			inside_comment
		);

	function_with_exported_name
		.detached_name(false)
		.set_error_message("You need to define the export name for your function.")
		.set_handle_tokens({ file_tokens::value_assign })
		.set_redirection_for_token(
			file_tokens::value_assign,
			generic_parser::state_action::change_top,
			inside_value
		);

	inside_value
		.detached_name(false)
		.set_error_message("Incorrectly set arguments types.")
		.set_handle_tokens({ file_tokens::new_line, file_tokens::end_of_file })
		.set_redirection_for_token(
			file_tokens::new_line,
			generic_parser::state_action::change_top,
			base_state
		);

	inside_comment
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			file_tokens::new_line,
			generic_parser::state_action::change_top,
			base_state
		);

	builder.attach_states(&this->parse_map);
}