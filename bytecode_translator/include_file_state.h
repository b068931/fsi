#ifndef INCLUDE_FILE_STATE_H
#define INCLUDE_FILE_STATE_H

#include <algorithm>
#include <iostream>
#include <vector>
#include <filesystem>
#include <format>
#include <exception>
#include <string_view>

#include "parser_options.h"
#include "type_definitions.h"

#include "../generic_parser/parser_facade.h"
#include "../startup_components/unicode_punning.h"

class include_file_state : public state_type {
public:
    void handle_token(
        structure_builder::file& output_file_structure,
        structure_builder::builder_parameters& helper,
        structure_builder::read_map_type& read_map
    ) override {
        auto active_parsing_files = read_map.get_parameters_container()
            .retrieve_parameter<std::shared_ptr<std::vector<std::filesystem::path>>>(
                structure_builder::parameters_enumeration::active_parsing_files
            );

        assert(active_parsing_files != nullptr && 
            !active_parsing_files->empty() && 
            "At least one active file for parsing must be present.");

        std::filesystem::path include_file{ 
            std::filesystem::canonical(UTF8_PATH(read_map.get_token_generator_name())) 
        };

        auto already_seen = std::ranges::find_if(
            *active_parsing_files,
            [&include_file](const std::filesystem::path& previous_file) -> bool {
                return std::filesystem::equivalent(previous_file, include_file);
            }
        );

        if (already_seen != active_parsing_files->end()) {
            read_map.exit_with_error(std::format("Cyclic dependency on {}.", already_seen->generic_string()));
            return;
        }

        // This is really should not be here but at the moment there is no other option.
        std::cout << "Now parsing: " << include_file.generic_string() << '\n';
        active_parsing_files->push_back(include_file);
        
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
            structure_builder::context_key::main_context,
            active_parsing_files
        };

        try {
            parser.start(include_file);
        }
        catch (const std::exception& exc) {
            read_map.exit_with_error(std::format("'{}': '{}'", include_file.generic_string(), exc.what()));
            return;
        }

        auto [line_error, error_string] = parser.error();
        if (!error_string.empty()) {
            read_map.exit_with_error(std::format(
                "'{}': '{} NEAR LINE {}'", include_file.generic_string(), 
                error_string, line_error));

            return;
        }

        auto names_stack = read_map
            .get_parameters_container()
            .retrieve_parameter<
                std::vector<std::pair<std::string, source_file_token>>*
            >(structure_builder::parameters_enumeration::names_stack);

        auto& other_names_stack = parser.get_names_stack();
        for (auto& [keyword, token] : other_names_stack) {
            auto predefined_keyword = std::ranges::find_if(
                parser_options::keywords,
                [&keyword](const auto& entry) {
                    return entry.first == keyword;
                }
            );

            if (predefined_keyword == parser_options::keywords.end()) {
                auto already_exists = std::ranges::find_if(
                    *names_stack,
                    [&keyword](const auto& entry) {
                        return entry.first == keyword;
                    }
                );

                if (already_exists != names_stack->end()) {
                    read_map.exit_with_error(std::format(
                        "Keyword '{}' already exists.",
                        keyword));

                    return;
                }

                names_stack->emplace_back(std::move(keyword), token);
            }
        }

        structure_builder::file parser_value{ parser.get_builder_value() };
        output_file_structure.modules.splice(output_file_structure.modules.end(), parser_value.modules);
        output_file_structure.exposed_functions.insert(
            output_file_structure.exposed_functions.end(),
            parser_value.exposed_functions.begin(),
            parser_value.exposed_functions.end()
        );

        for (const auto& string_name : parser_value.program_strings | std::views::keys) {
            auto found_string = output_file_structure.program_strings.find(string_name);
            if (found_string != output_file_structure.program_strings.end()) {
                read_map.exit_with_error(std::format(
                    "Duplicate program string {} found.",
                    found_string->first));

                return;
            }
        }

        output_file_structure.program_strings.merge(parser_value.program_strings);

        for (const structure_builder::function& other_function : parser_value.functions) {
            auto found_function = std::ranges::find_if(
                output_file_structure.functions,
                [&other_function](const structure_builder::function& entry) {
                    return entry.name == other_function.name;
                });

            if (found_function != output_file_structure.functions.end()) {
                read_map.exit_with_error(std::format(
                    "Function with name {} already exists.",
                    found_function->name));

                return;
            }
        }

        output_file_structure.functions.splice(output_file_structure.functions.end(), parser_value.functions);
        
        auto result = helper.name_translations.merge(
            parser.get_builder().get_names_translations());
        
        if (result.has_value()) {
            const auto& [other_key, other_value, this_key, this_value] = result.value();
            read_map.exit_with_error(std::format(
                "Failed to merge redefinition '{}'='{}', "
                "another '{}'='{}' already exists.",
                other_key, other_value, this_key, this_value));

            return;
        }

        active_parsing_files->pop_back();
        read_map.switch_context(structure_builder::context_key::main_context);
    }
};


#endif
