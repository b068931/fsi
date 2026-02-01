#ifndef INSIDE_FUNCTION_BODY_STATE_H
#define INSIDE_FUNCTION_BODY_STATE_H

#include "type_definitions.h"

class inside_function_body_state : public state_type {
public:
    void handle_token(
        structure_builder::file& output_file_structure,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        source_file_token token = read_map.get_current_token();
        if (token == source_file_token::function_arguments_start) { //special case for function call
            helper.active_function.add_new_instruction(source_file_token::function_call);
            helper.add_function_address_argument(output_file_structure, helper, read_map);

            ++helper.instruction_index; //used with jump points and jump instructions
        }
        else if (token == source_file_token::module_return_value) { //special case for module function call
            helper.active_function.add_new_instruction(source_file_token::module_call);
            helper.active_function.add_new_operand_to_last_instruction(
                source_file_token::no_return_module_call_keyword, nullptr, false);

            if (read_map.get_token_generator_additional_token() != source_file_token::no_return_module_call_keyword) {
                helper.active_function.map_operand_with_variable(
                    helper.name_translations.translate_name(read_map.get_token_generator_name()),
                    &std::get<1>(helper.active_function.get_last_operand()), read_map);
            }

            ++helper.instruction_index;
        }
        else if (token == source_file_token::function_body_end) {
            helper.active_function.set_current_function(nullptr);
            helper.instruction_index = 0;
        }
        else if (token != source_file_token::function_body_end && token != source_file_token::jump_point && token != source_file_token::endif_keyword && !read_map.is_token_generator_name_empty()) {
            helper.active_function.add_new_instruction(token);
            ++helper.instruction_index;
        }
        else if (token == source_file_token::expression_end && !read_map.is_token_generator_name_empty()) {
            read_map.exit_with_error();
        }
    }
};

#endif
