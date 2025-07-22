#ifndef NEW_JUMP_POINT_STATE_H
#define NEW_JUMP_POINT_STATE_H

#include <algorithm>

#include "type_definitions.h"

class new_jump_point_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string name = helper.name_translations.translate_name(read_map.get_token_generator_name());
		structure_builder::function& current_function = helper.active_function.get_current_function();

		auto found_jump_point =
            std::ranges::find_if(current_function.jump_points,
                                 [&name](const structure_builder::jump_point& jmp) {
                                     return jmp.name == name;
                                 });

		if (found_jump_point != current_function.jump_points.end()) { //set index if jump point was already created
			found_jump_point->index = static_cast<std::uint32_t>(helper.instruction_index);
		}
		else { //create new jump point otherwise
			current_function.jump_points.emplace_back(helper.get_id(), static_cast<std::uint32_t>(helper.instruction_index), name);
		}
	}
};

#endif