#include "structure_builder.h"
#include "type_definitions.h"
#include "module_import_state.h"
#include "functions_import_state.h"
#include "import_keyword_token_state.h"
#include "define_state.h"
#include "redefine_states.h"
#include "undefine_state.h"
#include "if_defined_states.h"
#include "stack_size_state.h"
#include "declaration_states.h"
#include "main_function_name_state.h"
#include "program_strings_states.h"
#include "function_address_argument_state.h"
#include "immediate_argument_states.h"
#include "sizeof_argument_state.h"
#include "regular_variable_argument_states.h"
#include "pointer_dereference_argument_states.h"
#include "jump_point_argument_state.h"
#include "string_argument_state.h"
#include "function_declaration_states.h"
#include "inside_function_body_state.h"
#include "new_jump_point_state.h"
#include "module_call_states.h"
#include "main_state.h"

std::vector<structure_builder::source_file_token> all_types {
	structure_builder::source_file_token::one_byte_type_keyword, structure_builder::source_file_token::two_bytes_type_keyword,
		structure_builder::source_file_token::four_bytes_type_keyword, structure_builder::source_file_token::eight_bytes_type_keyword,
		structure_builder::source_file_token::pointer_type_keyword
};

std::vector<structure_builder::source_file_token> integer_types {
	structure_builder::source_file_token::one_byte_type_keyword, structure_builder::source_file_token::two_bytes_type_keyword,
		structure_builder::source_file_token::four_bytes_type_keyword, structure_builder::source_file_token::eight_bytes_type_keyword
};

std::vector<structure_builder::source_file_token> argument_end_tokens {
	structure_builder::source_file_token::coma, structure_builder::source_file_token::function_args_end,
		structure_builder::source_file_token::expression_end
};

state_settings& configure_comment(states_builder_type& builder) {
	state_settings& inside_comment = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure,
			structure_builder::builder_parameters& helper,
			structure_builder::read_map_type& read_map) -> void {
				read_map
					.get_parameters_container()
					.assign_parameter(structure_builder::parameters_enumeration::has_just_left_comment, true);
		}
	)
		.set_handle_tokens({ structure_builder::source_file_token::comment_end })
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	return inside_comment;
}
state_settings& configure_modules_import(states_builder_type& builder, state_settings& comment) {
	state_settings& inside_functions_import = builder.create_state<functions_import_state>()
		.set_error_message("',' or '>' are expected, got another token instead.")
		.set_handle_tokens(
			{ 
				structure_builder::source_file_token::coma, 
				structure_builder::source_file_token::import_end, 
				structure_builder::source_file_token::comment_start 
			}
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::import_end,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);

	state_settings& import_start = builder.create_state<state_type>()
		.set_error_message("'<' was expected here.")
		.set_redirection_for_token(
			structure_builder::source_file_token::import_start,
			generic_parser::state_action::change_top,
			inside_functions_import
		);

	state_settings& import_keyword_token = builder.create_state<import_keyword_token_state>()
		.set_error_message("'import' was expected, got another token instead.")
		.set_handle_tokens({ structure_builder::source_file_token::import_start })
		.set_redirection_for_token(
			structure_builder::source_file_token::import_start,
			generic_parser::state_action::change_top,
			inside_functions_import
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::import_keyword,
			generic_parser::state_action::change_top,
			import_start
		);

	state_settings& inside_module_import = builder.create_state<module_import_state>()
		.set_error_message("Module name was expected, got another token instead.")
		.set_handle_tokens({ structure_builder::source_file_token::name })
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			import_keyword_token
		);

	return inside_module_import;
}

void configure_special_instruction_end(state_settings& last_state) {
	last_state
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		);
}
state_settings& configure_special_instructions(states_builder_type& builder) {
	auto ifdef_ifndef_pop_check_custom_function =
		[](generic_parser::parameters_container<structure_builder::parameters_enumeration>& parameters, structure_builder::builder_parameters& helper) -> bool {
		return parameters.retrieve_parameter<bool>(structure_builder::parameters_enumeration::ifdef_ifndef_pop_check);
	};

	state_settings& define = builder.create_state<define_state>()
		.set_error_message("';' was expected for $define.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& undefine = builder.create_state<undefine_state>()
		.set_error_message("';' was expected for $undefine.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& redefine_value = builder.create_state<redefine_value_state>()
		.set_error_message("';' was expected for $redefine.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& redefine_name = builder.create_state<redefine_name_state>()
		.set_error_message("$redefine expects two names, got another token instead.")
		.set_handle_tokens({ structure_builder::source_file_token::name })
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			redefine_value
		);

	state_settings& ignore_all_until_endif = builder.create_state<ignore_all_until_endif_state>()
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			structure_builder::source_file_token::endif_keyword,
			generic_parser::state_action::pop_top,
			nullptr
		);

	state_settings& if_defined = builder.create_state<if_defined_state>()
		.set_error_message("';' was expected for $if-defined.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end })
		.add_custom_function(
			ifdef_ifndef_pop_check_custom_function,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::change_top,
			ignore_all_until_endif
		);

	state_settings& if_not_defined = builder.create_state<if_not_defined_state>()
		.set_error_message("';' was expected for $if-not-defined")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end })
		.add_custom_function(
			ifdef_ifndef_pop_check_custom_function,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::change_top,
			ignore_all_until_endif
		);
		
	state_settings& stack_size = builder.create_state<stack_size_state>()
		.set_error_message("';' was expected for $stack-size.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& declare_name = builder.create_state<declare_name_state>()
		.set_error_message("';' was expected for $declare.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& declare_type = builder.create_state<declare_type_state>()
		.set_error_message("Unexpected type.")
		.set_handle_tokens(std::vector<structure_builder::source_file_token>{ all_types })
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			declare_name
		);

	state_settings& string_value = builder.create_state<string_value_state>()
		.set_handle_tokens({ structure_builder::source_file_token::string_separator })
		.set_redirection_for_token(
			structure_builder::source_file_token::string_separator,
			generic_parser::state_action::pop_top,
			nullptr
		);

	state_settings& string_name = builder.create_state<string_name_state>()
		.set_error_message("$string expects the name of the string, got another token instead.")
		.set_handle_tokens({ structure_builder::source_file_token::string_separator })
		.set_redirection_for_token(
			structure_builder::source_file_token::string_separator,
			generic_parser::state_action::change_top,
			string_value
		);

	state_settings& main_function_name = builder.create_state<main_function_name_state>()
		.set_error_message("$main-function expects the name of the function that will start the application.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end });

	state_settings& inside_special_instruction = builder.create_state<state_type>()
		.set_error_message("Invalid special instruction.")
		.set_redirection_for_token(
			structure_builder::source_file_token::define_keyword,
			generic_parser::state_action::change_top,
			define
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::redefine_keyword,
			generic_parser::state_action::change_top,
			redefine_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::undefine_keyword,
			generic_parser::state_action::change_top,
			undefine
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::if_defined_keyword,
			generic_parser::state_action::change_top,
			if_defined
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::if_not_defined_keyword,
			generic_parser::state_action::change_top,
			if_not_defined
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::stack_size_keyword,
			generic_parser::state_action::change_top,
			stack_size
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::declare_keyword,
			generic_parser::state_action::change_top,
			declare_type
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::define_string_keyword,
			generic_parser::state_action::change_top,
			string_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::main_function_keyword,
			generic_parser::state_action::change_top,
			main_function_name
		);

	configure_special_instruction_end(define);
	configure_special_instruction_end(undefine);
	configure_special_instruction_end(redefine_value);
	configure_special_instruction_end(stack_size);
	configure_special_instruction_end(declare_name);
	configure_special_instruction_end(main_function_name);

	return inside_special_instruction;
}

void configure_instruction_argument_end(state_settings& argument_last_state, state_settings& instruction_arguments_base, state_settings& comment) {
	argument_last_state
		.set_redirection_for_tokens(
			{
				structure_builder::source_file_token::function_args_end, structure_builder::source_file_token::expression_end
			},
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::coma,
			generic_parser::state_action::change_top,
			instruction_arguments_base
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		)
		.add_handle_token(structure_builder::source_file_token::comment_start);
}
state_settings& configure_instruction_arguments(states_builder_type& builder, state_settings& comment) {
	state_settings& function_address_argument = builder.create_state<function_address_argument_state>()
		.set_error_message("Unexpected token inside instruction. Function name was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ argument_end_tokens }
		);

	state_settings& immediate_argument_value = builder.create_state<immediate_argument_value_state>()
		.set_error_message("Unexpected token inside instruction. A value for an immediate argument was expected.")
		.set_handle_tokens(std::vector<structure_builder::source_file_token>{ argument_end_tokens });

	state_settings& immediate_argument_type = builder.create_state<immediate_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. Active type for an immediate value was expected.")
		.set_handle_tokens(std::vector<structure_builder::source_file_token>{ integer_types })
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			immediate_argument_value
		);

	state_settings& sizeof_argument = builder.create_state<sizeof_argument_state>()
		.set_error_message("Unexpected token inside instruction. A name of the string, type, etc. was expected for sizeof.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ argument_end_tokens }
		);

	state_settings& regular_variable_argument_name = builder.create_state<regular_variable_argument_name_state>()
		.set_error_message("Unexpected token inside instruction. A name of the regular variable was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ argument_end_tokens }
		);

	state_settings& regular_variable_argument_type = builder.create_state<regular_variable_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the regular variable was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ all_types }
		)
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			regular_variable_argument_name
		);

	state_settings& signed_regular_variable_argument = builder.create_state<signed_regualar_variable_argument_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the regular variable was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types }
		)
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			regular_variable_argument_name
		);

	state_settings& dereference_variables_add_end = builder.create_state<state_type>();
	state_settings& add_new_dereference_variable = builder.create_state<pointer_dereference_argument_dereference_variables_state>()
		.set_error_message("Unexpected token inside instruction. ',', or ']' were expected.")
		.set_handle_tokens(
			{
				structure_builder::source_file_token::coma,
				structure_builder::source_file_token::dereference_end,
				structure_builder::source_file_token::comment_start
			}
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::dereference_end,
			generic_parser::state_action::change_top,
			dereference_variables_add_end
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);

	state_settings& map_dereferenced_variable_name = builder.create_state<pointer_dereference_argument_pointer_name_state>()
		.set_error_message("Unexpected token inside instruction. ('[' expected)")
		.set_handle_tokens({ structure_builder::source_file_token::dereference_start })
		.set_redirection_for_token(
			structure_builder::source_file_token::dereference_start,
			generic_parser::state_action::change_top,
			add_new_dereference_variable
		);

	state_settings& pointer_dereference_argument_type = builder.create_state<pointer_dereference_argument_type_state>()
		.set_error_message("Unexpected token inside instruction. An active type for the pointer dereference was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types }
		)
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			map_dereferenced_variable_name
		);

	state_settings& add_jump_data_argument = builder.create_state<jump_point_argument_state>()
		.set_error_message("Unexpected token inside instruction. A name of the jump point was expected.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ argument_end_tokens }
		);

	state_settings& add_string_argument = builder.create_state<string_argument_state>()
		.set_error_message("Unexpected token inside instruction.")
		.set_handle_tokens(
			std::vector<structure_builder::source_file_token>{ argument_end_tokens }
		);

	state_settings& instruction_arguments_base = builder.create_state<state_type>()
		.set_error_message("Unexpected token inside instruction. You were expected to introduce a keyword for another arugment.")
		.set_redirection_for_tokens(
			{
				structure_builder::source_file_token::expression_end, structure_builder::source_file_token::function_args_end
			},
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_address_argument_keyword,
			generic_parser::state_action::change_top,
			function_address_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::immediate_argument_keyword,
			generic_parser::state_action::change_top,
			immediate_argument_type
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::variable_argument_keyword,
			generic_parser::state_action::change_top,
			regular_variable_argument_type
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::signed_argument_keyword,
			generic_parser::state_action::change_top,
			signed_regular_variable_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::pointer_dereference_argument_keyword,
			generic_parser::state_action::change_top,
			pointer_dereference_argument_type
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::jump_point_argument_keyword,
			generic_parser::state_action::change_top,
			add_jump_data_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::sizeof_argument_keyword,
			generic_parser::state_action::change_top,
			sizeof_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::string_argument_keyword,
			generic_parser::state_action::change_top,
			add_string_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);

	configure_instruction_argument_end(function_address_argument, instruction_arguments_base, comment);
	configure_instruction_argument_end(sizeof_argument, instruction_arguments_base, comment);
	configure_instruction_argument_end(immediate_argument_value, instruction_arguments_base, comment);
	configure_instruction_argument_end(regular_variable_argument_name, instruction_arguments_base, comment);
	configure_instruction_argument_end(dereference_variables_add_end, instruction_arguments_base, comment);
	configure_instruction_argument_end(add_jump_data_argument, instruction_arguments_base, comment);
	configure_instruction_argument_end(add_string_argument, instruction_arguments_base, comment);

	return instruction_arguments_base;
}

state_settings& configure_function_declaration(states_builder_type& builder, state_settings& function_body, state_settings& comment) {
	state_settings& declaration_or_definition = builder.create_state<declaration_or_definition_state>()
		.set_error_message("Expected ';' or '{'.")
		.set_handle_tokens(
			{ structure_builder::source_file_token::function_body_start }
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_body_start,
			generic_parser::state_action::change_top,
			function_body
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);

	state_settings& function_argument_name = builder.create_state<function_argument_name_state>()
		.set_error_message("Unexpected token inside function arguments. A name for a function argument was expected.")
		.set_handle_tokens(
			{
				structure_builder::source_file_token::coma,
				structure_builder::source_file_token::function_args_end,
				structure_builder::source_file_token::comment_start
			}
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_end,
			generic_parser::state_action::change_top,
			declaration_or_definition
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);


	state_settings& function_argument_type = builder.create_state<function_argument_type_state>()
		.set_error_message("Unexpected type for a function argument.")
		.set_handle_tokens(std::vector<structure_builder::source_file_token>{ all_types })
		.add_handle_token(structure_builder::source_file_token::function_args_end)
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			function_argument_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_end,
			generic_parser::state_action::change_top,
			declaration_or_definition
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		);

	state_settings& function_name = builder.create_state<function_name_state>()
		.set_error_message("Invalid function name.")
		.set_handle_tokens({ structure_builder::source_file_token::function_args_start })
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_start,
			generic_parser::state_action::change_top,
			function_argument_type
		);

	function_argument_name
		.set_redirection_for_token(
			structure_builder::source_file_token::coma,
			generic_parser::state_action::change_top,
			function_argument_type
		);
		
	return function_name;
}
state_settings& configure_inside_function(states_builder_type& builder, state_settings& comment, state_settings& special_instruction) {
	state_settings& instruction_arguments = configure_instruction_arguments(builder, comment);
	state_settings& new_jump_point = builder.create_state<new_jump_point_state>()
		.set_error_message("';' was expected.")
		.set_handle_tokens({ structure_builder::source_file_token::expression_end })
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	state_settings& module_call_function_name = builder.create_state<module_call_function_name_state>()
		.set_error_message("A name of one of the module functions was expected.")
		.set_handle_tokens({ structure_builder::source_file_token::function_args_start })
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_start,
			generic_parser::state_action::change_top,
			instruction_arguments
		);

	state_settings& module_call_name = builder.create_state<module_call_name_state>()
		.set_error_message("A name of one of the imported modules was expected.")
		.set_handle_tokens({ structure_builder::source_file_token::module_call })
		.set_redirection_for_token(
			structure_builder::source_file_token::module_call,
			generic_parser::state_action::change_top,
			module_call_function_name
		);

	state_settings& inside_function_body = builder.create_state<inside_function_body_state>()
		.set_error_message("Unexpected token inside function. You were expected to introduce a jump point, instruction, function or module call, etc.")
		.set_handle_tokens( //jump_point and function_body_end are useless? -- i have zero idea what this thing means after several months (or years?) of it being here
				{ //it is actually kind of funny that I did not manage to come up with something better than this
				structure_builder::source_file_token::function_args_start, structure_builder::source_file_token::expression_end,
				structure_builder::source_file_token::module_return_value, structure_builder::source_file_token::jump_point,
				structure_builder::source_file_token::function_body_end, structure_builder::source_file_token::add_instruction_keyword,
				structure_builder::source_file_token::signed_add_instruction_keyword, structure_builder::source_file_token::multiply_instruction_keyword,
				structure_builder::source_file_token::signed_multiply_instruction_keyword, structure_builder::source_file_token::subtract_instruction_keyword,
				structure_builder::source_file_token::signed_subtract_instruction_keyword, structure_builder::source_file_token::divide_instruction_keyword,
				structure_builder::source_file_token::signed_divide_instruction_keyword, structure_builder::source_file_token::compare_instruction_keyword,
				structure_builder::source_file_token::increment_instruction_keyword, structure_builder::source_file_token::decrement_instruction_keyword,
				structure_builder::source_file_token::jump_instruction_keyword, structure_builder::source_file_token::jump_greater_instruction_keyword,
				structure_builder::source_file_token::jump_greater_equal_instruction_keyword, structure_builder::source_file_token::jump_equal_instruction_keyword,
				structure_builder::source_file_token::jump_not_equal_instruction_keyword, structure_builder::source_file_token::jump_less_instruction_keyword,
				structure_builder::source_file_token::jump_less_equal_instruction_keyword, structure_builder::source_file_token::jump_above_instruction_keyword,
				structure_builder::source_file_token::jump_above_equal_instruction_keyword, structure_builder::source_file_token::jump_below_instruction_keyword,
				structure_builder::source_file_token::jump_below_equal_instruction_keyword, structure_builder::source_file_token::move_instruction_keyword,
				structure_builder::source_file_token::bit_and_instruction_keyword, structure_builder::source_file_token::bit_or_instruction_keyword,
				structure_builder::source_file_token::bit_xor_instruction_keyword, structure_builder::source_file_token::bit_not_instruction_keyword,
				structure_builder::source_file_token::load_value_instruction_keyword, structure_builder::source_file_token::save_value_instruction_keyword,
				structure_builder::source_file_token::move_pointer_instruction_keyword, structure_builder::source_file_token::bit_shift_left_instruction_keyword,
				structure_builder::source_file_token::bit_shift_right_instruction_keyword, structure_builder::source_file_token::get_function_address_instruction_keyword,
				structure_builder::source_file_token::copy_string_instruction_keyword
			}
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_start,
			generic_parser::state_action::push_state,
			instruction_arguments
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::special_instruction,
			generic_parser::state_action::push_state,
			special_instruction
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::jump_point,
			generic_parser::state_action::push_state,
			new_jump_point
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::module_return_value,
			generic_parser::state_action::push_state,
			module_call_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_body_end,
			generic_parser::state_action::pop_top,
			nullptr
		)
		.set_redirection_for_tokens(
			{
				structure_builder::source_file_token::add_instruction_keyword, structure_builder::source_file_token::signed_add_instruction_keyword, structure_builder::source_file_token::multiply_instruction_keyword,
				structure_builder::source_file_token::signed_multiply_instruction_keyword, structure_builder::source_file_token::subtract_instruction_keyword, structure_builder::source_file_token::signed_subtract_instruction_keyword,
				structure_builder::source_file_token::divide_instruction_keyword, structure_builder::source_file_token::signed_divide_instruction_keyword, structure_builder::source_file_token::compare_instruction_keyword,
				structure_builder::source_file_token::increment_instruction_keyword, structure_builder::source_file_token::decrement_instruction_keyword, structure_builder::source_file_token::jump_instruction_keyword,
				structure_builder::source_file_token::jump_not_equal_instruction_keyword, structure_builder::source_file_token::jump_equal_instruction_keyword, structure_builder::source_file_token::jump_greater_instruction_keyword,
				structure_builder::source_file_token::jump_greater_equal_instruction_keyword, structure_builder::source_file_token::jump_less_instruction_keyword, structure_builder::source_file_token::jump_less_equal_instruction_keyword,
				structure_builder::source_file_token::jump_above_instruction_keyword, structure_builder::source_file_token::jump_above_equal_instruction_keyword, structure_builder::source_file_token::jump_below_instruction_keyword,
				structure_builder::source_file_token::move_instruction_keyword, structure_builder::source_file_token::bit_and_instruction_keyword, structure_builder::source_file_token::bit_or_instruction_keyword,
				structure_builder::source_file_token::bit_xor_instruction_keyword, structure_builder::source_file_token::bit_not_instruction_keyword, structure_builder::source_file_token::save_value_instruction_keyword,
				structure_builder::source_file_token::load_value_instruction_keyword, structure_builder::source_file_token::move_pointer_instruction_keyword, structure_builder::source_file_token::bit_shift_left_instruction_keyword,
				structure_builder::source_file_token::bit_shift_right_instruction_keyword, structure_builder::source_file_token::get_function_address_instruction_keyword, structure_builder::source_file_token::copy_string_instruction_keyword
			},
			generic_parser::state_action::push_state,
			instruction_arguments
		);
		
	return inside_function_body;
}

void structure_builder::configure_parse_map() {
	states_builder_type builder{};

	state_settings& comment = configure_comment(builder);
	state_settings& modules_import = configure_modules_import(builder, comment);
	state_settings& special_instruction = configure_special_instructions(builder);
	state_settings& function_body = configure_inside_function(builder, comment, special_instruction);
	state_settings& function_declaration = configure_function_declaration(builder, function_body, comment);
	
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
			source_file_token::comment_start,
			generic_parser::state_action::push_state,
			comment
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