#ifndef JUMP_POINT_ARGUMENT_STATE_H
#define JUMP_POINT_ARGUMENT_STATE_H

#include <algorithm>
#include <limits>

#include "type_definitions.h"

class jump_point_argument_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        std::string name = helper.name_translations.translate_name(read_map.get_token_generator_name());
        if (name.empty()) {
            read_map.exit_with_error("Expected the name of the jump point, got another token instead.");
            return;
        }

        structure_builder::function& current_function = helper.active_function.get_current_function();
        auto found_jump_point =
            std::ranges::find_if(current_function.jump_points,
                                 [&name](const structure_builder::jump_point& jmp) {
                                     return jmp.name == name;
                                 });

        //simply create new jump point if it wasn't already created
        structure_builder::jump_point* jump_point;
        if (found_jump_point == current_function.jump_points.end()) {
            current_function.jump_points.emplace_back(helper.get_id(), std::numeric_limits<std::uint32_t>::max(), name);
            jump_point = &current_function.jump_points.back();
        }
        else {
            jump_point = &*found_jump_point;
        }

        helper.active_function.get_last_instruction().jump_variables.emplace_back(jump_point);
        helper.active_function.add_new_operand_to_last_instruction(
            source_file_token::jump_point_argument_keyword,
            &helper.active_function.get_last_instruction().jump_variables.back(),
            false
        );
    }
};

#endif
