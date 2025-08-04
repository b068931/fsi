#include "function_declaration_states.h"
#include "inside_function_body_state.h"
#include "new_jump_point_state.h"
#include "module_call_states.h"
#include "parser_options.h"

extern state_settings& configure_instruction_arguments(states_builder_type& builder);

extern state_settings& configure_function_declaration(states_builder_type& builder, state_settings& function_body) {
	state_settings& declaration_or_definition = builder.create_state<declaration_or_definition_state>()
		.set_error_message("Expected ';' or '{'.")
		.set_handle_tokens(
			{ source_file_token::function_body_start }
		)
		.set_redirection_for_token(
			source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			source_file_token::function_body_start,
			generic_parser::state_action::change_top,
			function_body
		);

	state_settings& function_argument_name = builder.create_state<function_argument_name_state>()
		.set_error_message("Unexpected token inside function arguments. A name for a function argument was expected.")
		.set_handle_tokens(
			{
				source_file_token::coma,
				source_file_token::function_arguments_end,
				source_file_token::comment_start
			}
		)
		.set_redirection_for_token(
			source_file_token::function_arguments_end,
			generic_parser::state_action::change_top,
			declaration_or_definition
		);

	state_settings& function_argument_type = builder.create_state<function_argument_type_state>()
		.set_error_message("Unexpected type for a function argument.")
		.set_handle_tokens(std::vector<source_file_token>{ parser_options::all_types })
		.add_handle_token(source_file_token::function_arguments_end)
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::all_types },
			generic_parser::state_action::change_top,
			function_argument_name
		)
		.set_redirection_for_token(
			source_file_token::function_arguments_end,
			generic_parser::state_action::change_top,
			declaration_or_definition
		);

	state_settings& function_name = builder.create_state<function_name_state>()
		.set_error_message("Invalid function name.")
		.set_handle_tokens({ source_file_token::function_arguments_start })
		.set_redirection_for_token(
			source_file_token::function_arguments_start,
			generic_parser::state_action::change_top,
			function_argument_type
		);

	function_argument_name
		.set_redirection_for_token(
			source_file_token::coma,
			generic_parser::state_action::change_top,
			function_argument_type
		);
		
	return function_name;
}

extern state_settings& configure_inside_function(states_builder_type& builder, state_settings& special_instruction) {
	state_settings& instruction_arguments = configure_instruction_arguments(builder);
	state_settings& new_jump_point = builder.create_state<new_jump_point_state>()
		.set_error_message("';' was expected.")
		.set_handle_tokens({ source_file_token::expression_end })
		.set_redirection_for_token(
			source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	state_settings& module_call_function_name = builder.create_state<module_call_function_name_state>()
		.set_error_message("A name of one of the module functions was expected.")
		.set_handle_tokens({ source_file_token::function_arguments_start })
		.set_redirection_for_token(
			source_file_token::function_arguments_start,
			generic_parser::state_action::change_top,
			instruction_arguments
		);

	state_settings& module_call_name = builder.create_state<module_call_name_state>()
		.set_error_message("A name of one of the imported modules was expected.")
		.set_handle_tokens({ source_file_token::module_call })
		.set_redirection_for_token(
			source_file_token::module_call,
			generic_parser::state_action::change_top,
			module_call_function_name
		);

	state_settings& inside_function_body = builder.create_state<inside_function_body_state>()
		.set_error_message("Unexpected token inside function. You were expected to introduce a jump point, instruction, function or module call, etc.")
		.set_handle_tokens( //jump_point and function_body_end are useless? -- i have zero idea what this thing means after several months (or years?) of it being here
				{ //it is actually kind of funny that I did not manage to come up with something better than this
				source_file_token::function_arguments_start, source_file_token::expression_end,
				source_file_token::module_return_value, source_file_token::jump_point,
				source_file_token::function_body_end, source_file_token::add_instruction_keyword,
				source_file_token::signed_add_instruction_keyword, source_file_token::multiply_instruction_keyword,
				source_file_token::signed_multiply_instruction_keyword, source_file_token::subtract_instruction_keyword,
				source_file_token::signed_subtract_instruction_keyword, source_file_token::divide_instruction_keyword,
				source_file_token::signed_divide_instruction_keyword, source_file_token::compare_instruction_keyword,
				source_file_token::increment_instruction_keyword, source_file_token::decrement_instruction_keyword,
				source_file_token::jump_instruction_keyword, source_file_token::jump_greater_instruction_keyword,
				source_file_token::jump_greater_equal_instruction_keyword, source_file_token::jump_equal_instruction_keyword,
				source_file_token::jump_not_equal_instruction_keyword, source_file_token::jump_less_instruction_keyword,
				source_file_token::jump_less_equal_instruction_keyword, source_file_token::jump_above_instruction_keyword,
				source_file_token::jump_above_equal_instruction_keyword, source_file_token::jump_below_instruction_keyword,
				source_file_token::jump_below_equal_instruction_keyword, source_file_token::move_instruction_keyword,
				source_file_token::bit_and_instruction_keyword, source_file_token::bit_or_instruction_keyword,
				source_file_token::bit_xor_instruction_keyword, source_file_token::bit_not_instruction_keyword,
				source_file_token::load_value_instruction_keyword, source_file_token::save_value_instruction_keyword,
				source_file_token::move_pointer_instruction_keyword, source_file_token::bit_shift_left_instruction_keyword,
				source_file_token::bit_shift_right_instruction_keyword, source_file_token::get_function_address_instruction_keyword,
				source_file_token::copy_string_instruction_keyword
			}
		)
		.set_redirection_for_token(
			source_file_token::function_arguments_start,
			generic_parser::state_action::push_state,
			instruction_arguments
		)
		.set_redirection_for_token(
			source_file_token::special_instruction,
			generic_parser::state_action::push_state,
			special_instruction
		)
		.set_redirection_for_token(
			source_file_token::jump_point,
			generic_parser::state_action::push_state,
			new_jump_point
		)
		.set_redirection_for_token(
			source_file_token::module_return_value,
			generic_parser::state_action::push_state,
			module_call_name
		)
		.set_redirection_for_token(
			source_file_token::function_body_end,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_tokens(
			{
				source_file_token::add_instruction_keyword, source_file_token::signed_add_instruction_keyword, source_file_token::multiply_instruction_keyword,
				source_file_token::signed_multiply_instruction_keyword, source_file_token::subtract_instruction_keyword, source_file_token::signed_subtract_instruction_keyword,
				source_file_token::divide_instruction_keyword, source_file_token::signed_divide_instruction_keyword, source_file_token::compare_instruction_keyword,
				source_file_token::increment_instruction_keyword, source_file_token::decrement_instruction_keyword, source_file_token::jump_instruction_keyword,
				source_file_token::jump_not_equal_instruction_keyword, source_file_token::jump_equal_instruction_keyword, source_file_token::jump_greater_instruction_keyword,
				source_file_token::jump_greater_equal_instruction_keyword, source_file_token::jump_less_instruction_keyword, source_file_token::jump_less_equal_instruction_keyword,
				source_file_token::jump_above_instruction_keyword, source_file_token::jump_above_equal_instruction_keyword, source_file_token::jump_below_instruction_keyword,
				source_file_token::move_instruction_keyword, source_file_token::bit_and_instruction_keyword, source_file_token::bit_or_instruction_keyword,
				source_file_token::bit_xor_instruction_keyword, source_file_token::bit_not_instruction_keyword, source_file_token::save_value_instruction_keyword,
				source_file_token::load_value_instruction_keyword, source_file_token::move_pointer_instruction_keyword, source_file_token::bit_shift_left_instruction_keyword,
				source_file_token::bit_shift_right_instruction_keyword, source_file_token::get_function_address_instruction_keyword, source_file_token::copy_string_instruction_keyword
			},
			generic_parser::state_action::push_state,
			instruction_arguments
		);
		
	return inside_function_body;
}
