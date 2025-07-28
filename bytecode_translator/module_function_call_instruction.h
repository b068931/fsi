#ifndef MODULE_FUNCTION_CALL_INSTRUCTION_FILTER_H
#define MODULE_FUNCTION_CALL_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct module_function_call_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::module_function_call };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.modules.size() == 1 && instruction.module_functions.size() == 1;
    }
};

#endif
