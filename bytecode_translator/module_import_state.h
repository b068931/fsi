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
		std::string module_name = helper.names_remapping.translate_name(read_map.get_token_generator_name());
		auto found_module = std::find_if(
			output_file_structure.modules.begin(),
			output_file_structure.modules.end(),
			[&module_name](const structure_builder::engine_module& mod) -> bool {
				return mod.name == module_name;
			}
		);

		if (found_module == output_file_structure.modules.end()) {
			output_file_structure.modules.push_front(
				structure_builder::engine_module{ helper.get_id(), std::move(module_name) }
			);

			helper.current_module = output_file_structure.modules.begin();
		}
		else {
			helper.current_module = found_module;
		}
	}
};

#endif