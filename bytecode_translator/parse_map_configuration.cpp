#include "structure_builder.h"
#include "../dll_mediator/fsi_types.h"

using states_builder_type = generic_parser::states_builder<structure_builder::source_file_token, structure_builder::context_key, structure_builder::file, structure_builder::helper_inter_states_object, structure_builder::parameters_enumeration>;
using state_settings = states_builder_type::state_settings_type;
using state_type = states_builder_type::state_type;

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

state_settings& configure_modules_import(states_builder_type& builder) {
	state_settings& inside_module_import = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			output_file_structure.modules.push_back(
				structure_builder::module{
					helper.get_id(),
					helper.names_remapping.translate_name(read_map.get_token_generator_name())
				}
			);
		}
	);

	state_settings& IMPORT_token = builder.create_state<state_type>();
	state_settings& import_start = builder.create_state<state_type>();
	state_settings& inside_functions_import = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			output_file_structure.modules.back().functions_names
				.push_back(
					structure_builder::module_function{
						helper.get_id(),
						helper.names_remapping.translate_name(read_map.get_token_generator_name())
					}
			);
		}
	);

	inside_module_import.set_handle_tokens({ structure_builder::source_file_token::name });
	inside_functions_import.set_handle_tokens({ structure_builder::source_file_token::coma, structure_builder::source_file_token::import_end });

	inside_module_import
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			IMPORT_token
		);

	IMPORT_token
		.set_redirection_for_token(
			structure_builder::source_file_token::import_keyword,
			generic_parser::state_action::change_top,
			import_start
		);

	import_start
		.set_redirection_for_token(
			structure_builder::source_file_token::import_start,
			generic_parser::state_action::change_top,
			inside_functions_import
		);

	inside_functions_import
		.set_redirection_for_token(
			structure_builder::source_file_token::import_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	inside_module_import.set_error_message("Module name was expected, got another token instead.");
	IMPORT_token.set_error_message("'import' was expected, got another token instead.");
	import_start.set_error_message("'<' was expected here.");
	inside_functions_import.set_error_message("',' or '>' are expected, got another token instead.");

	return inside_module_import;
}
state_settings& configure_comment(states_builder_type& builder) {
	state_settings& inside_comment = builder.create_state<state_type>();
	inside_comment
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			structure_builder::source_file_token::comment_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	return inside_comment;
}

void configure_special_instruction_end(state_settings& last_state) {
	last_state
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::pop_top,
			nullptr
		);
}
state_settings& configure_special_instructions(states_builder_type& builder) {
	state_settings& inside_special_instruction = builder.create_state<state_type>();
	state_settings& define = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.names_remapping.add(read_map.get_token_generator_name(), "");
		}
	);

	state_settings& redefine_name = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.names_remapping.add(read_map.get_token_generator_name(), "");
		}
	);
	state_settings& redefine_value = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.names_remapping.back().second = read_map.get_token_generator_name();
		}
	);

	state_settings& undefine = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.names_remapping.remove(read_map.get_token_generator_name());
		}
	);

	state_settings& ifdef = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			read_map
				.get_parameters_container()
				.assign_parameter(
					structure_builder::parameters_enumeration::ifdef_ifndef_pop_check,
					helper.names_remapping.has_remapping(read_map.get_token_generator_name())
				);
		}
	);

	state_settings& ifndef = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			read_map
				.get_parameters_container()
				.assign_parameter(
					structure_builder::parameters_enumeration::ifdef_ifndef_pop_check,
					!helper.names_remapping.has_remapping(read_map.get_token_generator_name())
				);
		}
	);

	state_settings& ignore_all_until_endif = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			read_map
				.get_parameters_container()
				.assign_parameter(structure_builder::parameters_enumeration::ifdef_ifndef_pop_check, false);
		}
	);

	state_settings& stack_size = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			try {
				output_file_structure.stack_size = helper.names_remapping.translate_name_to_integer<size_t>(
					read_map.get_token_generator_name()
				);
			}
			catch (const std::exception& exc) {
				read_map.exit_with_error(exc.what());
			}
		}
	);

	state_settings& decl_TYPE_name = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			if (helper.current_function.is_current_function_present()) {
				helper.current_function.get_current_function().locals
					.push_back(
						structure_builder::regular_variable{
							helper.get_id(),
							read_map.get_current_token()
						}
				);
			}
			else {
				read_map.exit_with_error();
			}
		}
	);

	state_settings& decl_type_NAME = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.get_current_function().locals.back().name =
				helper.names_remapping.translate_name(read_map.get_token_generator_name());
		}
	);

	state_settings& string_NAME_value = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			auto[iterator, is_inserted] = output_file_structure.program_strings.insert(
				{ read_map.get_token_generator_name(), structure_builder::string{ helper.get_id() } }
			);

			if (!is_inserted) {
				read_map.exit_with_error("String with key '" + iterator->first + "' already exists.");
				return;
			}

			helper.current_string = std::move(iterator);
			read_map.switch_context(structure_builder::context_key::inside_string);
		}
	);
	state_settings& string_name_VALUE = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_string->second.value = read_map.get_token_generator_name();
			read_map.switch_context(structure_builder::context_key::main_context);
		}
	);

	auto ifdef_ifndef_pop_check_custom_function =
		[](generic_parser::parameters_container<structure_builder::parameters_enumeration>& parameters, structure_builder::helper_inter_states_object& helper) -> bool {
		return parameters.retrieve_parameter<bool>(structure_builder::parameters_enumeration::ifdef_ifndef_pop_check);
	};

	ifdef.add_custom_function(
		ifdef_ifndef_pop_check_custom_function,
		generic_parser::state_action::pop_top,
		nullptr
	);

	ifndef.add_custom_function(
		ifdef_ifndef_pop_check_custom_function,
		generic_parser::state_action::pop_top,
		nullptr
	);

	define.set_handle_tokens({ structure_builder::source_file_token::name });
	redefine_name.set_handle_tokens({ structure_builder::source_file_token::name });
	redefine_value.set_handle_tokens({ structure_builder::source_file_token::name });
	undefine.set_handle_tokens({ structure_builder::source_file_token::name });
	ifdef.set_handle_tokens({ structure_builder::source_file_token::name });
	ifndef.set_handle_tokens({ structure_builder::source_file_token::name });
	stack_size.set_handle_tokens({ structure_builder::source_file_token::name });

	decl_TYPE_name.set_handle_tokens(std::vector<structure_builder::source_file_token>{ all_types });
	decl_type_NAME.set_handle_tokens({ structure_builder::source_file_token::name });

	string_NAME_value.set_handle_tokens({ structure_builder::source_file_token::string_separator });
	string_name_VALUE.set_handle_tokens({ structure_builder::source_file_token::string_separator });

	inside_special_instruction
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
			ifdef
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::if_not_defined_keyword,
			generic_parser::state_action::change_top,
			ifndef
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::stack_size_keyword,
			generic_parser::state_action::change_top,
			stack_size
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::declare_keyword,
			generic_parser::state_action::change_top,
			decl_TYPE_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::define_string_keyword,
			generic_parser::state_action::change_top,
			string_NAME_value
		);

	configure_special_instruction_end(define);
	configure_special_instruction_end(undefine);

	configure_special_instruction_end(redefine_value);
	redefine_name
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			redefine_value
		);

	ifdef
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			ignore_all_until_endif
		);

	ifndef
		.set_redirection_for_token(
			structure_builder::source_file_token::name,
			generic_parser::state_action::change_top,
			ignore_all_until_endif
		);

	ignore_all_until_endif
		.detached_name(false)
		.whitelist(false)
		.set_redirection_for_token(
			structure_builder::source_file_token::endif_keyword,
			generic_parser::state_action::pop_top,
			nullptr
		);

	configure_special_instruction_end(stack_size);

	configure_special_instruction_end(decl_type_NAME);
	decl_TYPE_name
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			decl_type_NAME
		);

	string_NAME_value
		.set_redirection_for_token(
			structure_builder::source_file_token::string_separator,
			generic_parser::state_action::change_top,
			string_name_VALUE
		);

	string_name_VALUE
		.set_redirection_for_token(
			structure_builder::source_file_token::string_separator,
			generic_parser::state_action::pop_top,
			nullptr
		);

	inside_special_instruction.set_error_message("Invalid special instruction.");
	define.set_error_message("$define expects one name, got another token instead.");
	redefine_name.set_error_message("$redefine expects two names, got another token instead.");
	redefine_value.set_error_message("$redefine expects two names, got another token instead.");
	undefine.set_error_message("$undefine expects one name, got another token instead.");
	ifdef.set_error_message("$ifdef expects only one name, got another token instead.");
	ifndef.set_error_message("$ifndef expects only one name, got another token instead.");
	stack_size.set_error_message("$stack_size expects only one number.");
	decl_TYPE_name.set_error_message("Unexpected type.");
	decl_type_NAME.set_error_message("$decl expects a type and only one name.");
	string_NAME_value.set_error_message("$string expects the name of the string, got another token instead.");

	return inside_special_instruction;
}

void configure_instruction_argument_end(state_settings& argument_last_state, state_settings& instruction_arguments_base) {
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
		);
}
state_settings& configure_instruction_arguments(states_builder_type& builder) {
	state_settings& instruction_arguments_base = builder.create_state<state_type>();
	state_settings& add_function_address_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.add_function_address_argument(output_file_structure, helper, read_map);
		}
	);

	state_settings& add_immediate_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.get_last_instruction().immediates.push_back(structure_builder::imm_variable{ read_map.get_current_token() });
			helper.current_function.add_new_operand_to_last_instruction(
				read_map.get_current_token(),
				&helper.current_function.get_last_instruction().immediates.back(),
				false
			);
		}
	);
	state_settings& add_immediate_argument_value = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			try {
				helper.current_function.get_last_instruction().immediates.back().imm_val =
					helper.names_remapping.translate_name_to_integer<structure_builder::immediate_type>(read_map.get_token_generator_name());
			}
			catch (const std::exception& exc) {
				read_map.exit_with_error(exc.what());
			}
		}
	);

	state_settings& add_sizeof_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			structure_builder::immediate_type element_size{};
			structure_builder::source_file_token element_token = read_map.get_token_generator_additional_token();

			std::string variable_name{};
			if (element_token == structure_builder::source_file_token::end_of_file) {
				variable_name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
				structure_builder::function& current_function = helper.current_function.get_current_function();

				auto found_local = helper.current_function.find_local_variable_by_name(variable_name);
				if (found_local != current_function.locals.end()) {
					element_token = found_local->type;
				}
				else {
					auto found_argument = helper.current_function.find_argument_variable_by_name(variable_name);
					if (found_argument != current_function.arguments.end()) {
						element_token = found_argument->type;
					}
				}
			}

			switch (element_token) {
			case structure_builder::source_file_token::one_byte_type_keyword:
				element_size = sizeof(module_mediator::one_byte);
				break;

			case structure_builder::source_file_token::two_bytes_type_keyword:
				element_size = sizeof(module_mediator::two_bytes);
				break;

			case structure_builder::source_file_token::four_bytes_type_keyword:
				element_size = sizeof(module_mediator::four_bytes);
				break;

			case structure_builder::source_file_token::eight_bytes_type_keyword:
				element_size = sizeof(module_mediator::eight_bytes);
				break;

			default:
				auto found_string = output_file_structure.program_strings.find(variable_name);
				if (found_string != output_file_structure.program_strings.end()) {
					element_size = found_string->second.value.size() + 1;
					break;
				}

				read_map.exit_with_error();
				return;
			}

			helper.current_function.get_last_instruction().immediates.push_back(
				structure_builder::imm_variable{ structure_builder::source_file_token::eight_bytes_type_keyword, element_size }
			);

			helper.current_function.add_new_operand_to_last_instruction(
				structure_builder::source_file_token::eight_bytes_type_keyword,
				&helper.current_function.get_last_instruction().immediates.back(),
				false
			);
		}
	);

	state_settings& add_regular_variable_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.add_new_operand_to_last_instruction(read_map.get_current_token(), nullptr, false);
		}
	);
	state_settings& add_regular_variable_argument_name = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.map_operand_with_variable(
				helper.names_remapping.translate_name(read_map.get_token_generator_name()),
				&std::get<1>(helper.current_function.get_last_operand()),
				read_map
			);
		}
	);

	state_settings& add_signed_regular_variable_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.add_new_operand_to_last_instruction(read_map.get_current_token(), nullptr, true);
		}
	);

	state_settings& add_pointer_dereference_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.get_last_instruction().dereferences.push_back(structure_builder::pointer_dereference{});
			helper.current_function.add_new_operand_to_last_instruction(
				read_map.get_current_token(),
				&helper.current_function.get_last_instruction().dereferences.back(),
				false
			);
		}
	);
	state_settings& map_dereferenced_variable_name = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.map_operand_with_variable(
				helper.names_remapping.translate_name(read_map.get_token_generator_name()),
				&helper.current_function.get_last_instruction().dereferences.back().pointer_variable,
				read_map
			);
		}
	);
	state_settings& add_new_dereference_variable = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			helper.current_function.get_last_instruction().dereferences.back().derefernce_indexes.push_back(nullptr);
			helper.current_function.map_operand_with_variable(
				helper.names_remapping.translate_name(read_map.get_token_generator_name()),
				&helper.current_function.get_last_instruction().dereferences.back().derefernce_indexes.back(),
				read_map
			);
		}
	);
	state_settings& dereference_variables_add_end = builder.create_state<state_type>();

	state_settings& add_jump_data_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
			structure_builder::function& current_function = helper.current_function.get_current_function();

			auto found_jump_point =
				std::find_if(current_function.jump_points.begin(), current_function.jump_points.end(),
					[&name](const structure_builder::jump_point& jmp) {
						return jmp.name == name;
					}
			);

			structure_builder::jump_point* jump_point = nullptr; //simply create new jump point if it wasn't already created
			if (found_jump_point == current_function.jump_points.end()) {
				current_function.jump_points.push_back({ helper.get_id(), static_cast<uint32_t>(helper.instruction_index - 1), name });
				jump_point = &current_function.jump_points.back();
			}
			else {
				jump_point = &(*found_jump_point);
			}

			helper.current_function.get_last_instruction().jump_variables.push_back({ jump_point });
			helper.current_function.add_new_operand_to_last_instruction(
				structure_builder::source_file_token::jump_point_argument_keyword,
				&helper.current_function.get_last_instruction().jump_variables.back(),
				false
			);
		}
	);

	state_settings& add_string_argument = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string string_name = read_map.get_token_generator_name();
			auto found_string = output_file_structure.program_strings.find(string_name);
			if (found_string == output_file_structure.program_strings.end()) {
				read_map.exit_with_error("String with name '" + string_name + "' does not exist.");
				return;
			}

			helper.current_function.get_last_instruction().strings.push_back({ &(found_string->second) });
			helper.current_function.add_new_operand_to_last_instruction(
				structure_builder::source_file_token::string_argument_keyword,
				&helper.current_function.get_last_instruction().strings.back(),
				false
			);
		}
	);

	add_function_address_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ argument_end_tokens }
	);

	add_immediate_argument.set_handle_tokens(std::vector<structure_builder::source_file_token>{ integer_types });
	add_immediate_argument_value.set_handle_tokens(std::vector<structure_builder::source_file_token>{ argument_end_tokens });

	add_sizeof_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ argument_end_tokens }
	);

	add_regular_variable_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ all_types }
	);
	add_regular_variable_argument_name.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ argument_end_tokens }
	);

	add_signed_regular_variable_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ integer_types }
	);

	add_pointer_dereference_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ integer_types }
	);
	map_dereferenced_variable_name.set_handle_tokens({ structure_builder::source_file_token::dereference_start });
	add_new_dereference_variable.set_handle_tokens(
		{ structure_builder::source_file_token::coma, structure_builder::source_file_token::dereference_end }
	);

	add_jump_data_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ argument_end_tokens }
	);

	add_string_argument.set_handle_tokens(
		std::vector<structure_builder::source_file_token>{ argument_end_tokens }
	);

	instruction_arguments_base
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
			add_function_address_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::immediate_argument_keyword,
			generic_parser::state_action::change_top,
			add_immediate_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::variable_argument_keyword,
			generic_parser::state_action::change_top,
			add_regular_variable_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::signed_argument_keyword,
			generic_parser::state_action::change_top,
			add_signed_regular_variable_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::pointer_dereference_argument_keyword,
			generic_parser::state_action::change_top,
			add_pointer_dereference_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::jump_point_argument_keyword,
			generic_parser::state_action::change_top,
			add_jump_data_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::sizeof_argument_keyword,
			generic_parser::state_action::change_top,
			add_sizeof_argument
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::string_argument_keyword,
			generic_parser::state_action::change_top,
			add_string_argument
		);

	configure_instruction_argument_end(add_function_address_argument, instruction_arguments_base);
	configure_instruction_argument_end(add_sizeof_argument, instruction_arguments_base);

	configure_instruction_argument_end(add_immediate_argument_value, instruction_arguments_base);
	add_immediate_argument
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			add_immediate_argument_value
		);

	configure_instruction_argument_end(add_regular_variable_argument_name, instruction_arguments_base);
	add_regular_variable_argument
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			add_regular_variable_argument_name
		);

	add_signed_regular_variable_argument
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			add_regular_variable_argument_name
		);

	add_pointer_dereference_argument
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ integer_types },
			generic_parser::state_action::change_top,
			map_dereferenced_variable_name
		);

	map_dereferenced_variable_name
		.set_redirection_for_token(
			structure_builder::source_file_token::dereference_start,
			generic_parser::state_action::change_top,
			add_new_dereference_variable
		);

	add_new_dereference_variable
		.set_redirection_for_token(
			structure_builder::source_file_token::dereference_end,
			generic_parser::state_action::change_top,
			dereference_variables_add_end
		);

	configure_instruction_argument_end(dereference_variables_add_end, instruction_arguments_base);
	configure_instruction_argument_end(add_jump_data_argument, instruction_arguments_base);

	configure_instruction_argument_end(add_string_argument, instruction_arguments_base);

	instruction_arguments_base.set_error_message("Unexpected token inside instruction.");
	add_function_address_argument.set_error_message("Unexpected token inside instruction.");
	add_immediate_argument.set_error_message("Unexpected token inside instruction.");
	add_immediate_argument_value.set_error_message("Unexpected token inside instruction.");
	add_sizeof_argument.set_error_message("Unexpected token inside instruction.");
	add_regular_variable_argument.set_error_message("Unexpected token inside instruction.");
	add_regular_variable_argument_name.set_error_message("Unexpected token inside instruction.");
	add_signed_regular_variable_argument.set_error_message("Unexpected token inside instruction.");
	add_pointer_dereference_argument.set_error_message("Unexpected token inside instruction.");
	map_dereferenced_variable_name.set_error_message("Unexpected token inside instruction. ('[' expected)");
	add_new_dereference_variable.set_error_message("Unexpected token inside instruction.");
	add_jump_data_argument.set_error_message("Unexpected token inside instruction.");
	add_string_argument.set_error_message("Unexpected token inside instruction.");

	return instruction_arguments_base;
}

state_settings& configure_function_declaration(states_builder_type& builder) {
	state_settings& declare_function = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			output_file_structure.functions.push_back(
				structure_builder::function{
					helper.get_id(),
					helper.names_remapping.translate_name(read_map.get_token_generator_name())
				}
			);
		}
	);

	state_settings& add_function_argument_type = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			if (read_map.get_current_token() == structure_builder::source_file_token::function_args_end) {
				if (!read_map.is_token_generator_name_empty()) {
					read_map.exit_with_error("Specify a name/type for your function argument.");
					return;
				}
			}
			else {
				output_file_structure.functions.back().arguments
					.push_back(
						structure_builder::regular_variable{
							helper.get_id(),
							read_map.get_current_token()
						}
				);
			}
		}
	);

	state_settings& add_function_argument_name = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string generated_name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
			if (!generated_name.empty()) {
				output_file_structure.functions.back().arguments.back().name =
					std::move(generated_name);
			}
			else {
				read_map.exit_with_error("Specify a name for your function argument.");
			}
		}
	);

	declare_function.set_handle_tokens({ structure_builder::source_file_token::function_args_start });
	add_function_argument_type.set_handle_tokens(std::vector<structure_builder::source_file_token>{ all_types }).add_handle_token(structure_builder::source_file_token::function_args_end);
	add_function_argument_name.set_handle_tokens({ structure_builder::source_file_token::coma, structure_builder::source_file_token::function_args_end });

	declare_function
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_start,
			generic_parser::state_action::change_top,
			add_function_argument_type
		);

	add_function_argument_type
		.set_redirection_for_tokens(
			std::vector<structure_builder::source_file_token>{ all_types },
			generic_parser::state_action::change_top,
			add_function_argument_name
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	add_function_argument_name
		.set_redirection_for_token(
			structure_builder::source_file_token::coma,
			generic_parser::state_action::change_top,
			add_function_argument_type
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	declare_function.set_error_message("Invalid function name.");
	add_function_argument_type.set_error_message("Unexpected type for function argument.");
	add_function_argument_name.set_error_message("Unexpected token inside function arguments.");

	return declare_function;
}
state_settings& configure_inside_function(states_builder_type& builder, state_settings& comment, state_settings& special_instruction) {
	state_settings& inside_function_body = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			structure_builder::source_file_token token = read_map.get_current_token();
			if (token == structure_builder::source_file_token::function_args_start) { //special case for function call
				helper.current_function.add_new_instruction(structure_builder::source_file_token::function_call);
				helper.add_function_address_argument(output_file_structure, helper, read_map);

				++helper.instruction_index; //used with jump points and jump instructions
			}
			else if (token == structure_builder::source_file_token::module_return_value) { //special case for module function call
				std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());

				helper.current_function.add_new_instruction(structure_builder::source_file_token::module_call);
				helper.current_function.add_new_operand_to_last_instruction(structure_builder::source_file_token::no_return_module_call_keyword, nullptr, false);
				if (name != "void") {
					helper.current_function.map_operand_with_variable(name, &std::get<1>(helper.current_function.get_last_operand()), read_map);
				}

				++helper.instruction_index;
			}
			else if (token == structure_builder::source_file_token::function_body_end) {
				helper.current_function.set_current_function(nullptr);
				helper.instruction_index = 0;
			}
			else if ((token != structure_builder::source_file_token::function_body_end) && (token != structure_builder::source_file_token::jump_point) && (token != structure_builder::source_file_token::endif_keyword)) {
				helper.current_function.add_new_instruction(token);
				++helper.instruction_index;
			}
		}
	);

	state_settings& instruction_arguments = configure_instruction_arguments(builder);
	state_settings& add_new_jump_point = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
			structure_builder::function& current_function = helper.current_function.get_current_function();

			auto found_jump_point =
				std::find_if(current_function.jump_points.begin(), current_function.jump_points.end(),
					[&name](const structure_builder::jump_point& jmp) {
						return jmp.name == name;
					}
			);

			if (found_jump_point != current_function.jump_points.end()) { //set index if jump point was already created
				found_jump_point->index = static_cast<uint32_t>(helper.instruction_index);
			}
			else { //create new jump point otherwise
				current_function.jump_points.push_back({ helper.get_id(), static_cast<uint32_t>(helper.instruction_index), name });
			}
		}
	);

	state_settings& add_module_name_to_instruction = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());

			auto found_module = std::find_if(output_file_structure.modules.begin(), output_file_structure.modules.end(),
				[&name](const structure_builder::module& mod) {
					return mod.name == name;
				}
			);
			if (found_module != output_file_structure.modules.end()) {
				helper.current_function.get_last_instruction().modules.push_back({ &(*found_module) });
				helper.current_function.add_new_operand_to_last_instruction(
					structure_builder::source_file_token::module_call,
					&helper.current_function.get_last_instruction().modules.back(),
					false
				);
			}
			else {
				read_map.exit_with_error("You did not import the module with name '" + name + "'.");
			}
		}
	);
	state_settings& add_module_function_name_to_instruction = builder.create_anonymous_state(
		[](structure_builder::file& output_file_structure, structure_builder::helper_inter_states_object& helper, structure_builder::read_map_type& read_map) -> void {
			std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
			structure_builder::variable* module_var = std::get<1>(helper.current_function.get_last_operand());

			structure_builder::module* referenced_module = static_cast<structure_builder::module_variable*>(module_var)->mod; //it is guaranteed that previous variable has module_variable type
			auto found_module_function = std::find_if(referenced_module->functions_names.begin(), referenced_module->functions_names.end(),
				[&name](const structure_builder::module_function& mod) {
					return mod.name == name;
				}
			);

			if (found_module_function != referenced_module->functions_names.end()) {
				helper.current_function.get_last_instruction().module_functions.push_back({ &(*found_module_function) });
				helper.current_function.add_new_operand_to_last_instruction(
					structure_builder::source_file_token::module_call,
					&helper.current_function.get_last_instruction().module_functions.back(),
					false
				);
			}
			else {
				read_map.exit_with_error(
					"You did not import the function with name '" +
					name + "' from '" + referenced_module->name + "'."
				);
			}
		}
	);

	//jump_point and function_body_end are useless?
	inside_function_body.set_handle_tokens(
		{
			structure_builder::source_file_token::function_args_start,
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
	);

	add_new_jump_point.set_handle_tokens({ structure_builder::source_file_token::expression_end });
	add_module_name_to_instruction.set_handle_tokens({ structure_builder::source_file_token::module_call });
	add_module_function_name_to_instruction.set_handle_tokens({ structure_builder::source_file_token::function_args_start });

	inside_function_body
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
			add_new_jump_point
		)
		.set_redirection_for_token(
			structure_builder::source_file_token::module_return_value,
			generic_parser::state_action::push_state,
			add_module_name_to_instruction
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

	add_new_jump_point
		.set_redirection_for_token(
			structure_builder::source_file_token::expression_end,
			generic_parser::state_action::pop_top,
			nullptr
		);

	add_module_name_to_instruction
		.set_redirection_for_token(
			structure_builder::source_file_token::module_call,
			generic_parser::state_action::change_top,
			add_module_function_name_to_instruction
		);

	add_module_function_name_to_instruction
		.set_redirection_for_token(
			structure_builder::source_file_token::function_args_start,
			generic_parser::state_action::change_top,
			instruction_arguments
		);

	inside_function_body.set_error_message("Unexpected token inside function.");
	add_new_jump_point.set_error_message("Unexpected token inside instruction.");
	add_module_name_to_instruction.set_error_message("Unexpected token inside instruction.");
	add_module_function_name_to_instruction.set_error_message("Unexpected token inside instruction.");

	return inside_function_body;
}

void structure_builder::configure_parse_map() {
	states_builder_type builder{};
	state_settings& main_state = builder.create_anonymous_state(
		[](file& output_file_structure, helper_inter_states_object& helper, read_map_type& read_map) -> void {
			if (read_map.get_current_token() == source_file_token::function_body_start) {
				std::string name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
				auto found_function = std::find_if(output_file_structure.functions.begin(), output_file_structure.functions.end(),
					[&name](const function& function) -> bool {
						return function.name == name;
					}
				);

				if (found_function != output_file_structure.functions.end()) {
					helper.current_function.set_current_function(&(*found_function));
				}
				else {
					read_map.exit_with_error("Function '" + name + "' was not declared. Declare a function before defining its body.");
				}
			}
		}
	);

	state_settings& modules_import = configure_modules_import(builder);
	state_settings& comment = configure_comment(builder);
	state_settings& special_instruction = configure_special_instructions(builder);
	state_settings& function_declaration = configure_function_declaration(builder);
	state_settings& function_body = configure_inside_function(builder, comment, special_instruction);
	
	main_state.set_handle_tokens(
		{
			source_file_token::end_of_file,
			source_file_token::endif_keyword,
			source_file_token::function_body_start
		}
	);

	main_state.set_error_message("Invalid token outside function. Remove spaces between function name and '{' if any.");
	main_state
		.set_as_starting_state()
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