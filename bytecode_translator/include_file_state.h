#ifndef INCLUDE_FILE_STATE_H
#define INCLUDE_FILE_STATE_H

#include <algorithm>
#include <iostream>
#include <vector>
#include <filesystem>
#include <format>
#include <exception>

#include "parser_options.h"
#include "type_definitions.h"
#include "../generic_parser/parser_facade.h"

class include_file_state : public state_type {
public:
    inline static std::vector<std::filesystem::path> active_parsing_files{};

    void handle_token(
        structure_builder::file& output_file_structure,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        std::filesystem::path include_file{ std::filesystem::canonical(read_map.get_token_generator_name()) };
        auto already_seen = std::ranges::find_if(
            active_parsing_files,
            [&include_file](const std::filesystem::path& previous_file) -> bool {
                return std::filesystem::equivalent(previous_file, include_file);
            }
        );

        if (already_seen != active_parsing_files.end()) {
            read_map.exit_with_error(std::format("Cyclic dependency on {}.", already_seen->generic_string()));
            return;
        }

        //this is really should not be here but at the moment there is no other option
        std::cout << "Now parsing: " << include_file.generic_string() << '\n';
        active_parsing_files.push_back(include_file);
        
        generic_parser::parser_facade<
            source_file_token, 
            structure_builder::context_key, 
            structure_builder
        > parser
        {
            parser_options::keywords,
            parser_options::contexts,
            source_file_token::name,
            source_file_token::end_of_file,
            structure_builder::context_key::main_context
        };

        try {
            parser.start(include_file);
        }
        catch (const std::exception& exc) {
            read_map.exit_with_error(std::format("'{}': '{}'", include_file.generic_string(), exc.what()));
            return;
        }

        std::pair error{ parser.error() };
        if (!error.second.empty()) {
            read_map.exit_with_error(std::format("'{}': '{} NEAR LINE {}'", include_file.generic_string(), error.second, error.first));
            return;
        }

        structure_builder::file parser_value{ parser.get_builder_value() };
        output_file_structure.exposed_functions.insert(
            output_file_structure.exposed_functions.end(),
            parser_value.exposed_functions.begin(),
            parser_value.exposed_functions.end()
        );

        std::size_t expected_program_strings_size = output_file_structure.program_strings.size() + parser_value.program_strings.size();
        output_file_structure.program_strings.merge(parser_value.program_strings);
        if (output_file_structure.program_strings.size() != expected_program_strings_size) {
            read_map.exit_with_error("Could not correctly merge program strings with included file.");
        }

        output_file_structure.modules.splice(output_file_structure.modules.end(), parser_value.modules);
        output_file_structure.functions.splice(output_file_structure.functions.end(), parser_value.functions);
        helper.name_translations.merge(std::move(parser.get_builder().get_names_translations()));

        active_parsing_files.pop_back();
        read_map.switch_context(structure_builder::context_key::main_context);
    }
};


#endif
