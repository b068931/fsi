#ifndef STACK_SIZE_STATE_H
#define STACK_SIZE_STATE_H

#include "type_definitions.h"

class stack_size_state : public state_type {
public:
    void handle_token(
        structure_builder::file& output_file_structure,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        try {
            output_file_structure.stack_size = helper.name_translations.translate_name_to_integer<std::size_t>(
                read_map.get_token_generator_name()
            );
        }
        catch (const std::exception& exc) {
            read_map.exit_with_error(exc.what());
        }
    }
};

#endif
