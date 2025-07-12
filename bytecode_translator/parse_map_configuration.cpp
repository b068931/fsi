#include "structure_builder.h"
#include "type_definitions.h"
#include "main_state.h"

extern state_settings& configure_special_instructions(states_builder_type &builder);
extern state_settings& configure_modules_import(states_builder_type &builder);
extern state_settings& configure_function_declaration(states_builder_type& builder, state_settings& function_body);
extern state_settings& configure_inside_function(states_builder_type& builder, state_settings& special_instruction);

void structure_builder::configure_parse_map() {
	states_builder_type builder{};

	state_settings& modules_import = configure_modules_import(builder);
	state_settings& special_instruction = configure_special_instructions(builder);
	state_settings& function_body = configure_inside_function(builder, special_instruction);
	state_settings& function_declaration = configure_function_declaration(builder, function_body);
	
	state_settings& base = builder.create_state<main_state>()
		.set_as_starting_state()
		.set_error_message("Invalid token outside function.")
		.set_handle_tokens(
			{
				source_file_token::end_of_file,
				source_file_token::endif_keyword,
				source_file_token::expression_end,
				source_file_token::function_body_start
			}
		)
		.set_redirection_for_token(
			source_file_token::from_keyword,
			generic_parser::state_action::push_state,
			modules_import
		)
		.set_redirection_for_token(
			source_file_token::special_instruction,
			generic_parser::state_action::push_state,
			special_instruction
		)
		.set_redirection_for_token(
			source_file_token::function_declaration_keyword,
			generic_parser::state_action::push_state,
			function_declaration
		)
		.set_redirection_for_token(
			source_file_token::function_body_start,
			generic_parser::state_action::push_state,
			function_body
		);

	builder.attach_states(&this->parse_map);
}