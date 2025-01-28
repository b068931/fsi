#ifndef INSIDE_HEADER_STATE_H
#define INSIDE_HEADER_STATE_H

#include "type_definitions.h"

class inside_header_state : public state_type {
public:
	virtual void handle_token(
		dll_builder::result_type& dlls,
		dll_builder::builder_parameters& parameters,
		dll_builder::read_map_type& read_map
	) override {
		if (parameters.is_visible) {
			read_map.exit_with_error("You can not define the entire module as 'visible'.");
		}

		switch (read_map.get_current_token()) {
		case dll_builder::file_tokens::name_and_public_name_separator:
			parameters.module_name = read_map.get_token_generator_name();
			break;

		case dll_builder::file_tokens::header_close:
			dlls.push_back(
				dll{
					std::move(parameters.module_name),
					read_map.get_token_generator_name(),
					parameters.dll_part
				}
			);

			break;
		}
	}
};

#endif // !INSIDE_HEADER_STATE_H