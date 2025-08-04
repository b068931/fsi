#include "define_state.h"
#include "redefine_states.h"
#include "undefine_state.h"
#include "if_defined_states.h"
#include "stack_size_state.h"
#include "declaration_states.h"
#include "include_file_state.h"
#include "inside_special_instruction_state.h"
#include "main_function_name_state.h"
#include "program_strings_states.h"
#include "expose_function_state.h"

#include "parser_options.h"

namespace {
    void configure_special_instruction_end(state_settings &last_state) {
        last_state
            .set_redirection_for_token(
                source_file_token::expression_end,
                generic_parser::state_action::pop_top,
                nullptr
            );
    }
}

extern state_settings& configure_special_instructions(states_builder_type& builder) {
    auto if_defined_if_not_defined_pop_check_custom =
        [](generic_parser::parameters_container<structure_builder::parameters_enumeration>& parameters, structure_builder::builder_parameters& helper) -> bool {
        return parameters.retrieve_parameter<bool>(structure_builder::parameters_enumeration::if_defined_if_not_defined_pop_check);
    };

    state_settings& define = builder.create_state<define_state>()
        .set_error_message("';' was expected for $define.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& undefine = builder.create_state<undefine_state>()
        .set_error_message("';' was expected for $undefine.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& redefine_value = builder.create_state<redefine_value_state>()
        .set_error_message("';' was expected for $redefine.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& redefine_name = builder.create_state<redefine_name_state>()
        .set_error_message("$redefine expects two names, got another token instead.")
        .set_handle_tokens({ source_file_token::name })
        .set_redirection_for_token(
            source_file_token::name,
            generic_parser::state_action::change_top,
            redefine_value
        );

    state_settings& ignore_all_until_endif = builder.create_state<ignore_all_until_endif_state>()
        .detached_name(false)
        .whitelist(false)
        .set_redirection_for_token(
            source_file_token::endif_keyword,
            generic_parser::state_action::pop_top,
            nullptr
        );

    state_settings& if_defined = builder.create_state<if_defined_state>()
        .set_error_message("';' was expected for $if-defined.")
        .set_handle_tokens({ source_file_token::expression_end })
        .add_custom_function(
            if_defined_if_not_defined_pop_check_custom,
            generic_parser::state_action::pop_top,
            nullptr
        )
        .set_redirection_for_token(
            source_file_token::expression_end,
            generic_parser::state_action::change_top,
            ignore_all_until_endif
        );

    state_settings& if_not_defined = builder.create_state<if_not_defined_state>()
        .set_error_message("';' was expected for $if-not-defined")
        .set_handle_tokens({ source_file_token::expression_end })
        .add_custom_function(
            if_defined_if_not_defined_pop_check_custom,
            generic_parser::state_action::pop_top,
            nullptr
        )
        .set_redirection_for_token(
            source_file_token::expression_end,
            generic_parser::state_action::change_top,
            ignore_all_until_endif
        );
        
    state_settings& stack_size = builder.create_state<stack_size_state>()
        .set_error_message("';' was expected for $stack-size.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& declare_name = builder.create_state<declare_name_state>()
        .set_error_message("';' was expected for $declare.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& declare_type = builder.create_state<declare_type_state>()
        .set_error_message("Unexpected type.")
        .set_handle_tokens(std::vector<source_file_token>{ parser_options::all_types })
        .set_redirection_for_tokens(
            std::vector<source_file_token>{ parser_options::all_types },
            generic_parser::state_action::change_top,
            declare_name
        );

    state_settings& string_value = builder.create_state<string_value_state>()
        .set_handle_tokens({ source_file_token::string_separator })
        .set_redirection_for_token(
            source_file_token::string_separator,
            generic_parser::state_action::pop_top,
            nullptr
        );

    state_settings& string_name = builder.create_state<string_name_state>()
        .set_error_message("$string expects the name of the string, got another token instead.")
        .set_handle_tokens({ source_file_token::string_separator })
        .set_redirection_for_token(
            source_file_token::string_separator,
            generic_parser::state_action::change_top,
            string_value
        );

    state_settings& main_function_name = builder.create_state<main_function_name_state>()
        .set_error_message("$main-function expects the name of the function that will start the application.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& include_file = builder.create_state<include_file_state>()
        .set_error_message("';' was expected for include, got another token instead.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& expose_function = builder.create_state<expose_function_state>()
        .set_error_message("';' was expected for $expose-function, got another token instead.")
        .set_handle_tokens({ source_file_token::expression_end });

    state_settings& inside_special_instruction = builder.create_state<inside_special_instruction_state>()
        .set_error_message("Invalid special instruction.")
        .set_handle_tokens({ source_file_token::include_keyword })
        .set_redirection_for_token(
            source_file_token::define_keyword,
            generic_parser::state_action::change_top,
            define
        )
        .set_redirection_for_token(
            source_file_token::redefine_keyword,
            generic_parser::state_action::change_top,
            redefine_name
        )
        .set_redirection_for_token(
            source_file_token::undefine_keyword,
            generic_parser::state_action::change_top,
            undefine
        )
        .set_redirection_for_token(
            source_file_token::if_defined_keyword,
            generic_parser::state_action::change_top,
            if_defined
        )
        .set_redirection_for_token(
            source_file_token::if_not_defined_keyword,
            generic_parser::state_action::change_top,
            if_not_defined
        )
        .set_redirection_for_token(
            source_file_token::stack_size_keyword,
            generic_parser::state_action::change_top,
            stack_size
        )
        .set_redirection_for_token(
            source_file_token::declare_keyword,
            generic_parser::state_action::change_top,
            declare_type
        )
        .set_redirection_for_token(
            source_file_token::define_string_keyword,
            generic_parser::state_action::change_top,
            string_name
        )
        .set_redirection_for_token(
            source_file_token::main_function_keyword,
            generic_parser::state_action::change_top,
            main_function_name
        )
        .set_redirection_for_token(
            source_file_token::include_keyword,
            generic_parser::state_action::change_top,
            include_file
        )
        .set_redirection_for_token(
            source_file_token::expose_function_keyword,
            generic_parser::state_action::change_top,
            expose_function
        );

    configure_special_instruction_end(define);
    configure_special_instruction_end(undefine);
    configure_special_instruction_end(redefine_value);
    configure_special_instruction_end(stack_size);
    configure_special_instruction_end(declare_name);
    configure_special_instruction_end(main_function_name);
    configure_special_instruction_end(include_file);
    configure_special_instruction_end(expose_function);

    return inside_special_instruction;
}

