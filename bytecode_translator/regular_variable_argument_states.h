#ifndef REGULAR_VARIABLE_ARGUMENT_STATES_H
#define REGULAR_VARIABLE_ARGUMENT_STATES_H

#include "type_definitions.h"

class regular_variable_argument_type_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        helper.active_function.add_new_operand_to_last_instruction(read_map.get_current_token(), nullptr, false);
    }
};

class regular_variable_argument_name_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        std::string variable_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
        if (variable_name.empty()) {
            read_map.exit_with_error("Expected the name of the regular variable, got another token instead.");
            return;
        }

        helper.active_function.map_operand_with_variable(
            std::move(variable_name),
            &std::get<1>(helper.active_function.get_last_operand()),
            read_map
        );
    }
};

class signed_regualar_variable_argument_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        helper.active_function.add_new_operand_to_last_instruction(read_map.get_current_token(), nullptr, true);
    }
};

#endif
