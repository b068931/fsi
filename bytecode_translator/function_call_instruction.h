#ifndef FUNCTION_CALL_INSTRUCTION_FILTER_H
#define FUNCTION_CALL_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

inline constexpr auto max_instruction_arguments_count = 15;

struct function_call_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::function_call };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.operands_in_order.size() >= 0 && instruction.operands_in_order.size() <= max_instruction_arguments_count;
    }
};

#endif