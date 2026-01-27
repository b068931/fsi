#ifndef EXPOSE_FUNCTION_STATE_H
#define EXPOSE_FUNCTION_STATE_H

#include <format>
#include "type_definitions.h"

class expose_function_state : public state_type {
public:
	virtual void handle_token(
		structure_builder::file& output_file_structure,
		structure_builder::builder_parameters& helper,
		structure_builder::read_map_type& read_map
	) override {
	    std::string function_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
        auto found_exposed_function = std::ranges::find_if(output_file_structure.exposed_functions, 
                                                           [&function_name](const auto& func) {
                                                               return func->name == function_name;
                                                           });

        // Do nothing if the function is already exposed
        if (found_exposed_function == output_file_structure.exposed_functions.end()) {
            auto found_function = std::ranges::find_if(output_file_structure.functions,
                                                       [&function_name](const auto& func) {
                                                           return func.name == function_name;
                                                       });

            if (found_function != output_file_structure.functions.end()) {
                output_file_structure.exposed_functions.push_back(&*found_function);
            }
            else {
                read_map.exit_with_error(
                    std::format("Function '{}' does not exist.", function_name)
                );
            }
        }
	}
};


#endif
