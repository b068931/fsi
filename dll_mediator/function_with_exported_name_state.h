#ifndef FUNCTION_WITH_EXPORTED_NAME_STATE_H
#define FUNCTION_WITH_EXPORTED_NAME_STATE_H

#include "type_definitions.h"

class function_with_exported_name_state : public state_type {
	virtual void handle_token(
		dll_builder::result_type& dlls,
		dll_builder::builder_parameters& parameters,
		dll_builder::read_map_type& read_map
	) override {
		if (read_map.is_token_generator_name_empty()) {
			read_map.exit_with_error();
		}
		else {
			parameters.function_exported_name =
				read_map.get_token_generator_name();
		}
	}
};

#endif