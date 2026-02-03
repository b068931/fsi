#ifndef MAIN_STATE_H
#define MAIN_STATE_H

#include "type_definitions.h"

namespace module_mediator::parser::states {
    class main_state : public state_type {
    public:
        void handle_token(
            components::engine_module_builder::result_type&,
            components::engine_module_builder::builder_parameters& parameters,
            components::engine_module_builder::read_map_type& read_map
        ) override {
#ifdef __clang__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch-default"

#endif

            switch (read_map.get_current_token()) {
            case components::engine_module_builder::file_tokens::name_and_public_name_separator:
                parameters.function_name =
                    read_map.get_token_generator_name();

                break;

            case components::engine_module_builder::file_tokens::value_assign:
                parameters.function_name =
                    read_map.get_token_generator_name();

                parameters.function_exported_name = parameters.function_name;
                break;

            case components::engine_module_builder::file_tokens::program_callable_function:
                if (parameters.is_visible) {
                    read_map.exit_with_error("You can not define the same function as 'visible' two times in row.");
                }

                parameters.is_visible = true;
                break;
            }

#ifdef __clang__

#pragma clang diagnostic pop

#endif
        }
    };
}

#endif // !MAIN_STATE_H
