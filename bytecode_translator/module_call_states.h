#ifndef MODULE_CALL_STATES_H
#define MODULE_CALL_STATES_H

#include <algorithm>

#include "type_definitions.h"

class module_call_name_state : public state_type {
public:
    void handle_token(
        structure_builder::file& output_file_structure,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        std::string name = helper.name_translations.translate_name(read_map.get_token_generator_name());

        auto found_module = std::ranges::find_if(output_file_structure.modules,
                                                 [&name](const structure_builder::engine_module& mod) {
                                                     return mod.name == name;
                                                 });

        if (found_module != output_file_structure.modules.end()) {
            helper.active_function.get_last_instruction().modules.emplace_back(&*found_module);
            helper.active_function.add_new_operand_to_last_instruction(
                source_file_token::module_call,
                &helper.active_function.get_last_instruction().modules.back(),
                false
            );
        }
        else {
            read_map.exit_with_error("You did not import the module with name '" + name + "'.");
        }
    }
};

class module_call_function_name_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        std::string name = helper.name_translations.translate_name(read_map.get_token_generator_name());
        structure_builder::variable* module_var = std::get<1>(helper.active_function.get_last_operand());

        structure_builder::engine_module* referenced_module = static_cast<structure_builder::module_variable*>(module_var)->mod; //it is guaranteed that previous variable has module_variable type
        auto found_module_function = std::ranges::find_if(referenced_module->functions_names,
                                                          [&name](const structure_builder::module_function& mod) {
                                                              return mod.name == name;
                                                          }
        );

        if (found_module_function != referenced_module->functions_names.end()) {
            helper.active_function.get_last_instruction().module_functions.emplace_back(&*found_module_function);
            helper.active_function.add_new_operand_to_last_instruction(
                source_file_token::module_call,
                &helper.active_function.get_last_instruction().module_functions.back(),
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
};

#endif
