#include "parser_options.h"

const std::vector<source_file_token> parser_options::all_types {
    source_file_token::one_byte_type_keyword, source_file_token::two_bytes_type_keyword,
        source_file_token::four_bytes_type_keyword, source_file_token::eight_bytes_type_keyword,
        source_file_token::memory_type_keyword
};

const std::vector<source_file_token> parser_options::integer_types {
    source_file_token::one_byte_type_keyword, source_file_token::two_bytes_type_keyword,
        source_file_token::four_bytes_type_keyword, source_file_token::eight_bytes_type_keyword
};

const std::vector<source_file_token> parser_options::argument_end_tokens {
    source_file_token::coma, source_file_token::function_arguments_end,
        source_file_token::expression_end
};

const generic_parser::token_generator<
    source_file_token,
    structure_builder::context_key
>::symbols_pair parser_options::main_context{
    .hard_symbols = {
        {"/*", source_file_token::comment_start},
        {",", source_file_token::coma},
        {"$", source_file_token::special_instruction},
        {"<", source_file_token::import_start},
        {">", source_file_token::import_end},
        {"(", source_file_token::function_arguments_start},
        {")", source_file_token::function_arguments_end},
        {"{", source_file_token::function_body_start},
        {"}", source_file_token::function_body_end},
        {"[", source_file_token::dereference_start},
        {"]", source_file_token::dereference_end},
        {";", source_file_token::expression_end},
        {":", source_file_token::module_return_value},
        {"@", source_file_token::jump_point},
        {"->", source_file_token::module_call},
        {"$endif;", source_file_token::endif_keyword},
        {"\r\n", source_file_token::new_line}, //despite the fact that new line is a hard symbol, it won't be passed to the generic_builder. it is used to find the line with invalid syntax
        {"\n", source_file_token::new_line},
        {"''''", source_file_token::string_separator}
    },
    .separators = { " ", "\t" }
};

const generic_parser::token_generator<
    source_file_token,
    structure_builder::context_key
>::symbols_pair parser_options::inside_string{
    .hard_symbols = {
        {"''''", source_file_token::string_separator}
    },
    .separators = {}
};

const generic_parser::token_generator<
    source_file_token,
    structure_builder::context_key
>::symbols_pair parser_options::inside_include{
    .hard_symbols = {
        {";", source_file_token::expression_end}
    },
    .separators = {}
};

const generic_parser::token_generator<
    source_file_token,
    structure_builder::context_key
>::symbols_pair parser_options::inside_comment{
    .hard_symbols = {
        {"*/", source_file_token::comment_end}
    },
    .separators = {}
};

const std::vector<std::pair<std::string, source_file_token>> parser_options::keywords{
    { "from", source_file_token::from_keyword },
    { "import", source_file_token::import_keyword },
    { "function", source_file_token::function_declaration_keyword },

    { "redefine", source_file_token::redefine_keyword },
    { "define", source_file_token::define_keyword },
    { "undefine", source_file_token::undefine_keyword},
    { "if-defined", source_file_token::if_defined_keyword },
    { "if-not-defined", source_file_token::if_not_defined_keyword },
    { "end-if", source_file_token::endif_keyword },
    { "declare", source_file_token::declare_keyword },
    { "stack-size", source_file_token::stack_size_keyword },
    { "define-string", source_file_token::define_string_keyword },
    { "main-function", source_file_token::main_function_keyword },
    { "include", source_file_token::include_keyword },
    { "expose-function", source_file_token::expose_function_keyword },

    { "immediate", source_file_token::immediate_argument_keyword },
    { "signed", source_file_token::signed_argument_keyword },
    { "variable", source_file_token::variable_argument_keyword },
    { "dereference", source_file_token::pointer_dereference_argument_keyword },
    { "function-name", source_file_token::function_address_argument_keyword },
    { "point", source_file_token::jump_point_argument_keyword },
    { "string", source_file_token::string_argument_keyword },
    { "one-byte", source_file_token::one_byte_type_keyword },
    { "two-bytes", source_file_token::two_bytes_type_keyword },
    { "four-bytes", source_file_token::four_bytes_type_keyword },
    { "eight-bytes", source_file_token::eight_bytes_type_keyword },
    { "memory", source_file_token::memory_type_keyword },
    { "size-of", source_file_token::sizeof_argument_keyword },

    { "add", source_file_token::add_instruction_keyword },
    { "signed-add", source_file_token::signed_add_instruction_keyword },
    { "multiply", source_file_token::multiply_instruction_keyword },
    { "signed-multiply", source_file_token::signed_multiply_instruction_keyword },
    { "subtract", source_file_token::subtract_instruction_keyword },
    { "signed-subtract", source_file_token::signed_subtract_instruction_keyword },
    { "divide", source_file_token::divide_instruction_keyword },
    { "signed-divide", source_file_token::signed_divide_instruction_keyword },
    { "compare", source_file_token::compare_instruction_keyword },
    { "increment", source_file_token::increment_instruction_keyword },
    { "decrement", source_file_token::decrement_instruction_keyword },
    { "jump", source_file_token::jump_instruction_keyword },
    { "jump-equal", source_file_token::jump_equal_instruction_keyword },
    { "jump-not-equal", source_file_token::jump_not_equal_instruction_keyword },
    { "jump-greater", source_file_token::jump_greater_instruction_keyword },
    { "jump-greater-equal", source_file_token::jump_greater_equal_instruction_keyword },
    { "jump-less", source_file_token::jump_less_instruction_keyword },
    { "jump-less-equal", source_file_token::jump_less_equal_instruction_keyword },
    { "jump-above", source_file_token::jump_above_instruction_keyword },
    { "jump-above-equal", source_file_token::jump_above_equal_instruction_keyword },
    { "jump-below", source_file_token::jump_below_instruction_keyword },
    { "jump-below-equal", source_file_token::jump_below_equal_instruction_keyword },
    { "move", source_file_token::move_instruction_keyword },
    { "copy-string", source_file_token::copy_string_instruction_keyword },
    { "get-function-address", source_file_token::get_function_address_instruction_keyword },
    { "bit-and", source_file_token::bit_and_instruction_keyword },
    { "bit-or", source_file_token::bit_or_instruction_keyword },
    { "bit-xor", source_file_token::bit_xor_instruction_keyword },
    { "bit-not", source_file_token::bit_not_instruction_keyword },
    { "bit-shift-left", source_file_token::bit_shift_left_instruction_keyword },
    { "bit-shift-right", source_file_token::bit_shift_right_instruction_keyword },
    { "save-value", source_file_token::save_value_instruction_keyword },
    { "load-value", source_file_token::load_value_instruction_keyword },
    { "move-pointer", source_file_token::move_pointer_instruction_keyword },
    { "void", source_file_token::no_return_module_call_keyword }
};

const std::map<
    structure_builder::context_key,
    generic_parser::token_generator<
        source_file_token,
        structure_builder::context_key
    >::symbols_pair
> parser_options::contexts{
    {
        structure_builder::context_key::main_context,
        main_context
    },
    {
        structure_builder::context_key::inside_string,
        inside_string
    },
    {
        structure_builder::context_key::inside_include,
        inside_include
    },
    {
        structure_builder::context_key::inside_comment,
        inside_comment
    }
};
