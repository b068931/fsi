#include "module_import_state.h"
#include "functions_import_state.h"
#include "import_keyword_token_state.h"

#include "parser_options.h"

extern state_settings& configure_modules_import(states_builder_type& builder);
state_settings& configure_modules_import(states_builder_type& builder) {
	state_settings &inside_functions_import = builder.create_state<functions_import_state>()
		.set_error_message("',' or '>' are expected, got another token instead.")
		.set_handle_tokens(
			{
				source_file_token::coma,
				source_file_token::import_end,
				source_file_token::comment_start
			}
		)
		.set_redirection_for_token(
			source_file_token::import_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	state_settings& import_start = builder.create_state<state_type>()
		.set_error_message("'<' was expected here.")
		.set_redirection_for_token(
			source_file_token::import_start,
			generic_parser::state_action::change_top,
			inside_functions_import
		);

	state_settings& import_keyword_token = builder.create_state<import_keyword_token_state>()
		.set_error_message("'import' was expected, got another token instead.")
		.set_handle_tokens({ source_file_token::import_start })
		.set_redirection_for_token(
			source_file_token::import_start,
			generic_parser::state_action::change_top,
			inside_functions_import
		)
		.set_redirection_for_token(
			source_file_token::import_keyword,
			generic_parser::state_action::change_top,
			import_start
		);

	state_settings& inside_module_import = builder.create_state<module_import_state>()
		.set_error_message("Module name was expected, got another token instead.")
		.set_handle_tokens({ source_file_token::name })
		.set_redirection_for_token(
			source_file_token::name,
			generic_parser::state_action::change_top,
			import_keyword_token
		);

	return inside_module_import;
}

