#ifndef TRANSLATOR_ERROR_TYPE_H
#define TRANSLATOR_ERROR_TYPE_H

enum class translator_error_type {
    name_too_long,
    too_many_function_arguments,
    general_instruction,
    jump_instruction,
    data_instruction,
    var_instruction,
    apply_on_first_operand_instruction,
    same_type_instruction,
    binary_instruction,
    multi_instruction,
    different_type_instruction,
    different_type_multi_instruction,
    empty_instruction,
    pointer_instruction,
    function_call,
    program_function_call,
    module_function_call,
    save_variable_state_instruction,
    load_variable_state_instruction,
    pointer_ref_instruction,
    bit_shift_instruction,
    get_function_address_instruction,
    non_string_instruction,
    string_instruction,
    unknown_jump_point,
    main_not_exposed,
    unknown_instruction
};

inline void translate_error(translator_error_type error, std::ostream& stream) {
    switch (error) {
        case translator_error_type::general_instruction: {
            stream << "You can not use signed variables and function calls inside general instructions";
            break;
        }
        case translator_error_type::jump_instruction: {
            stream << "You can use ONLY one point variable inside 'jump' instructions";
            break;
        }
        case translator_error_type::data_instruction: {
            stream << "You can not use point variables inside data instructions";
            break;
        }
        case translator_error_type::var_instruction: {
            stream << "You can not use pointer ARGUMENTS inside var instructions";
            break;
        }
        case translator_error_type::apply_on_first_operand_instruction: {
            stream << "This instruction changes the value of the first operand, so you can not use immediate as the first operand here.";
            break;
        }
        case translator_error_type::same_type_instruction: {
            stream << "You can not use different active types inside same type instructions and you can not use less than two arguments. Also you can not use immediate data as your first argument";
            break;
        }
        case translator_error_type::binary_instruction: {
            stream << "Binary instructions use only one pair of arguments";
            break;
        }
        case translator_error_type::multi_instruction: {
            stream << "Multi instructions can use up to 15 arguments";
            break;
        }
        case translator_error_type::different_type_instruction: {
            stream << "Different type instructions can not use immediates";
            break;
        }
        case translator_error_type::different_type_multi_instruction: {
            stream << "Different type multi instructions can use up to 15 arguments";
            break;
        }
        case translator_error_type::empty_instruction: {
            stream << "You can not use arguments inside empty instruction";
            break;
        }
        case translator_error_type::pointer_instruction: {
            stream << "You can not use more than two arguments inside pointer instruction. Also you need to specify one argument with type POINTER";
            break;
        }
        case translator_error_type::function_call: {
            stream << "You can not use more than 250 arguments inside function call instruction";
            break;
        }
        case translator_error_type::program_function_call: {
            stream << "You can not use signed variables inside program function call";
            break;
        }
        case translator_error_type::module_function_call: {
            stream << "Invalid module function call";
            break;
        }
        case translator_error_type::save_variable_state_instruction: {
            stream << "save instruction takes only one argument, excluding signed variables, jump points, etc.";
            break;
        }
        case translator_error_type::load_variable_state_instruction: {
            stream << "load instruction takes only one argument, excluding immediates, signed variables, jump points, etc";
            break;
        }
        case translator_error_type::pointer_ref_instruction: {
            stream << "ref instruction must be used to store pointer value from one pointer variable to another or you can use it to store pointer value to another pointer's memory";
            break;
        }
        case translator_error_type::get_function_address_instruction: {
            stream << "get_function_address instruction must be used with fnc as the second argument while the first argument is the place to store the eight_bytes displacement";
            break;
        }
        case translator_error_type::non_string_instruction: {
            stream << "Only copy_string instruction can use string as the second argument";
            break;
        }
        case translator_error_type::string_instruction: {
            stream << "copy_string instruction uses 'str' as its second argument";
            break;
        }
        case translator_error_type::unknown_jump_point: {
            stream << "Jump point has been used, yet it has not been defined anywhere";
            break;
        }
        case translator_error_type::main_not_exposed: {
            stream << "You did not expose the main function. Execution engine won't be able to check its signature";
            break;
        }
        case translator_error_type::unknown_instruction: {
            stream << "Unknown instruction";
            break;
        }
        default: {
            stream << "Unknown error code";
            break;
        }
    }
}

#endif
