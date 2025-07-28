#ifndef ERROR_TYPE_H
#define ERROR_TYPE_H

//Keys for errors that may occur during the parsing of the source code.
enum class parser_error_type {
    no_error,
    outside_function,
    inside_module_import,
    import_was_expected,
    import_start_was_expected,
    coma_or_import_end_were_expected,
    special_symbol,
    define_name_expected,
    redefine_name_expected,
    if_defined_name_expected,
    if_not_defined_name_expected,
    stack_size_name_expected,
    invalid_number,
    unexpected_type,
    can_not_declare_variable,
    declare_name,
    function_name,
    arguments_start_expected,
    arguments_end_or_coma,
    function_arguments_unexpected_type,
    function_arguments_unexpected_token,
    function_arguments_empty_name,
    function_body_start_was_expected,
    function_body_token,
    instruction_token,
    name_does_not_exist
};

#endif
