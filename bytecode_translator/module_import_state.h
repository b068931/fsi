#ifndef MODULE_IMPORT_STATE_H
#define MODULE_IMPORT_STATE_H

#include "type_definitions.h"

class module_import_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
		output_file_structure.modules.push_back(
			structure_builder::engine_module{
				helper.get_id(),
				helper.names_remapping.translate_name(read_map.get_token_generator_name())
			}
		);
	}
};

#endif