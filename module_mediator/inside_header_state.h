#ifndef INSIDE_HEADER_STATE_H
#define INSIDE_HEADER_STATE_H

#include "type_definitions.h"

class inside_header_state : public state_type {
public:
	virtual void handle_token(
		engine_module_builder::result_type& modules,
		engine_module_builder::builder_parameters& parameters,
		engine_module_builder::read_map_type& read_map
	) override {
		if (parameters.is_visible) {
			read_map.exit_with_error("You can not define the entire module as 'visible'.");
		}

		switch (read_map.get_current_token()) {
		case engine_module_builder::file_tokens::name_and_public_name_separator:
			parameters.module_name = read_map.get_token_generator_name();
			break;

		case engine_module_builder::file_tokens::header_close:
			modules.push_back(
				engine_module{
					std::move(parameters.module_name),
					read_map.get_token_generator_name(),
					parameters.module_part
				}
			);

			break;
		}
	}
};

#endif // !INSIDE_HEADER_STATE_H