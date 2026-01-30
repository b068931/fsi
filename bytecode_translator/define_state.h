#ifndef DEFINE_STATE_H
#define DEFINE_STATE_H

#include <format>
#include "type_definitions.h"

class define_state : public state_type {
public:
	void handle_token(
		structure_builder::file&,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		std::string define_name = read_map.get_token_generator_name();
		if (helper.name_translations.has_remapping(define_name)) {
			read_map.exit_with_error(std::format("Name '{}' has already been defined.", define_name));
			return;
		}

		helper.name_translations.add(std::move(define_name), "");
	}
};

#endif
