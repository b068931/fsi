#ifndef INSIDE_HEADER_STATE_H
#define INSIDE_HEADER_STATE_H

#include "type_definitions.h"

namespace module_mediator::parser::states {
    class inside_header_state : public state_type {
    public:
        virtual void handle_token(
            components::engine_module_builder::result_type& modules,
            components::engine_module_builder::builder_parameters& parameters,
            components::engine_module_builder::read_map_type& read_map
        ) override {
            if (parameters.is_visible) {
                read_map.exit_with_error("You can not define the entire module as 'visible'.");
            }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch-default"

            switch (read_map.get_current_token()) {
            case components::engine_module_builder::file_tokens::name_and_public_name_separator:
                parameters.module_name = read_map.get_token_generator_name();
                break;

            case components::engine_module_builder::file_tokens::header_close:
                modules.emplace_back(
                    std::move(parameters.module_name),
                    read_map.get_token_generator_name(),
                    parameters.module_part

                );

                break;
            }
            
#pragma clang diagnostic pop
        }
    };
}

#endif // !INSIDE_HEADER_STATE_H
