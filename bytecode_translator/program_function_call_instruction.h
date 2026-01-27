#ifndef PROGRAM_FUNCTION_CALL_INSTRUCTION_FILTER_H
#define PROGRAM_FUNCTION_CALL_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct program_function_call_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::program_function_call };
    static bool check(const structure_builder::instruction& instruction) {
        if (instruction.modules.empty() && instruction.module_functions.empty()) {
            for (const auto& index : instruction.operands_in_order) {
                if (std::get<2>(index)) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }
};

#endif
