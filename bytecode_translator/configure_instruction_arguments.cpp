#include "function_address_argument_state.h"
#include "immediate_argument_states.h"
#include "sizeof_argument_states.h"
#include "regular_variable_argument_states.h"
#include "pointer_dereference_argument_states.h"
#include "jump_point_argument_state.h"
#include "string_argument_state.h"
#include "parser_options.h"

namespace {
	void configure_instruction_argument_end(state_settings& argument_last_state, state_settings& instruction_arguments_base) {
		argument_last_state
			.set_redirection_for_tokens(
				{
					source_file_token::function_arguments_end, source_file_token::expression_end
				},
				generic_parser::state_action::pop_top,
				nullptr
			)
			.set_redirection_for_token(
				source_file_token::coma,
				generic_parser::state_action::change_top,
				instruction_arguments_base
			)
			.add_handle_token(source_file_token::comment_start);
	}
}

extern state_settings& configure_instruction_arguments(states_builder_type& builder);
state_settings& configure_instruction_arguments(states_builder_type& builder) {
	state_settings& function_address_argument = builder.create_state<function_address_argument_state>()
		.set_error_message("Unexpected token inside instruction. Function name was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::argument_end_tokens }
		);

	state_settings& immediate_argument_value = builder.create_state<immediate_argument_value_state>()
		.set_error_message("Unexpected token inside instruction. A value for an immediate argument was expected.")
		.set_handle_tokens(std::vector<source_file_token>{ parser_options::argument_end_tokens });

	state_settings& immediate_argument_type = builder.create_state<immediate_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. Active type for an immediate value was expected.")
		.set_handle_tokens(std::vector<source_file_token>{ parser_options::integer_types })
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::integer_types },
			generic_parser::state_action::change_top,
			immediate_argument_value
		);

	state_settings& sizeof_argument_value = builder.create_state<sizeof_argument_value_state>()
		.set_error_message("Unexpected token inside instruction. A name of the string, type, etc. was expected for sizeof.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::argument_end_tokens }
		);

	state_settings& sizeof_argument_type = builder.create_state<sizeof_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. The size for the size-of argument was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::all_types }
		)
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::all_types },
			generic_parser::state_action::change_top,
			sizeof_argument_value
		);

	state_settings& regular_variable_argument_name = builder.create_state<regular_variable_argument_name_state>()
		.set_error_message("Unexpected token inside instruction. A name of the regular variable was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::argument_end_tokens }
		);

	state_settings& regular_variable_argument_type = builder.create_state<regular_variable_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the regular variable was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::all_types }
		)
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::all_types },
			generic_parser::state_action::change_top,
			regular_variable_argument_name
		);

	state_settings& signed_regular_variable_argument = builder.create_state<signed_regualar_variable_argument_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the regular variable was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::integer_types }
		)
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::integer_types },
			generic_parser::state_action::change_top,
			regular_variable_argument_name
		);

	state_settings& dereference_variables_add_end = builder.create_state<state_type>();
	state_settings& add_new_dereference_variable = builder.create_state<pointer_dereference_argument_dereference_variables_state>()
		.set_error_message("Unexpected token inside instruction. ',', or ']' were expected.")
		.set_handle_tokens(
			{
				source_file_token::coma,
				source_file_token::dereference_end,
				source_file_token::comment_start
			}
		)
		.set_redirection_for_token(
			source_file_token::dereference_end,
			generic_parser::state_action::change_top,
			dereference_variables_add_end
		);

	state_settings& map_dereferenced_variable_name = builder.create_state<pointer_dereference_argument_pointer_name_state>()
		.set_error_message("Unexpected token inside instruction. ('[' expected)")
		.set_handle_tokens({ source_file_token::dereference_start })
		.set_redirection_for_token(
			source_file_token::dereference_start,
			generic_parser::state_action::change_top,
			add_new_dereference_variable
		);

	state_settings& pointer_dereference_argument_type = builder.create_state<pointer_dereference_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the pointer dereference was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::integer_types }
		)
		.set_redirection_for_tokens(
			std::vector<source_file_token>{ parser_options::integer_types },
			generic_parser::state_action::change_top,
			map_dereferenced_variable_name
		);

	state_settings& add_jump_data_argument = builder.create_state<jump_point_argument_state>()
		.set_error_message("Unexpected token inside instruction. A name of the jump point was expected.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::argument_end_tokens }
		);

	state_settings& add_string_argument = builder.create_state<string_argument_state>()
		.set_error_message("Unexpected token inside instruction.")
		.set_handle_tokens(
			std::vector<source_file_token>{ parser_options::argument_end_tokens }
		);

	state_settings& instruction_arguments_base = builder.create_anonymous_state(
        [](structure_builder::file&,
                                     structure_builder::builder_parameters&,
                                     structure_builder::read_map_type& read_map) {
            // Discard expression as incorrect if there is a name between coma (,) and the end of expression (;)
            if (!read_map.is_token_generator_name_empty()) {
				read_map.exit_with_error();
            }
        }
	)
		.set_error_message("Unexpected token inside instruction. You were expected to introduce a keyword for another argument.")
		.add_handle_token(source_file_token::expression_end)
        .add_handle_token(source_file_token::function_arguments_end)
		.set_redirection_for_tokens(
			{
				source_file_token::expression_end, source_file_token::function_arguments_end
			},
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			source_file_token::function_address_argument_keyword,
			generic_parser::state_action::change_top,
			function_address_argument
		)
		.set_redirection_for_token(
			source_file_token::immediate_argument_keyword,
			generic_parser::state_action::change_top,
			immediate_argument_type
		)
		.set_redirection_for_token(
			source_file_token::variable_argument_keyword,
			generic_parser::state_action::change_top,
			regular_variable_argument_type
		)
		.set_redirection_for_token(
			source_file_token::signed_argument_keyword,
			generic_parser::state_action::change_top,
			signed_regular_variable_argument
		)
		.set_redirection_for_token(
			source_file_token::pointer_dereference_argument_keyword,
			generic_parser::state_action::change_top,
			pointer_dereference_argument_type
		)
		.set_redirection_for_token(
			source_file_token::jump_point_argument_keyword,
			generic_parser::state_action::change_top,
			add_jump_data_argument
		)
		.set_redirection_for_token(
			source_file_token::sizeof_argument_keyword,
			generic_parser::state_action::change_top,
			sizeof_argument_type
		)
		.set_redirection_for_token(
			source_file_token::string_argument_keyword,
			generic_parser::state_action::change_top,
			add_string_argument
		);

	configure_instruction_argument_end(function_address_argument, instruction_arguments_base);
	configure_instruction_argument_end(sizeof_argument_value, instruction_arguments_base);
	configure_instruction_argument_end(immediate_argument_value, instruction_arguments_base);
	configure_instruction_argument_end(regular_variable_argument_name, instruction_arguments_base);
	configure_instruction_argument_end(dereference_variables_add_end, instruction_arguments_base);
	configure_instruction_argument_end(add_jump_data_argument, instruction_arguments_base);
	configure_instruction_argument_end(add_string_argument, instruction_arguments_base);

	return instruction_arguments_base;
}
