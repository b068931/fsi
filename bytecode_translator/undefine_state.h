#ifndef UNDEFINE_STATE_H
#define UNDEFINE_STATE_H

#include "type_definitions.h"

class undefine_state : public state_type {
public:
    void handle_token(
        structure_builder::file&,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        helper.name_translations.remove(read_map.get_token_generator_name());
    }
};

#endif
