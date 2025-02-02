#ifndef INSIDE_FUNCTION_BODY_STATE_H
#define INSIDE_FUNCTION_BODY_STATE_H

#include "type_definitions.h"

class inside_function_body_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
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
		else if ((token != structure_builder::source_file_token::function_body_end) && (token != structure_builder::source_file_token::jump_point) && (token != structure_builder::source_file_token::endif_keyword) && (!read_map.is_token_generator_name_empty())) {
			helper.current_function.add_new_instruction(token);
			++helper.instruction_index;
		}
		else if ((token == structure_builder::source_file_token::expression_end) && (!read_map.is_token_generator_name_empty())) {
			read_map.exit_with_error();
		}
	}
};

#endif