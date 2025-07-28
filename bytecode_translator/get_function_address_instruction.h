#ifndef GET_FUNCTION_ADDRESS_INSTRUCTION_FILTER_H
#define GET_FUNCTION_ADDRESS_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct get_function_address_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::get_function_address_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        bool initial_check =
            instruction.modules.empty() && instruction.module_functions.empty();

        if (initial_check && instruction.operands_in_order.size() >= 2) {
            for (const auto& index : instruction.operands_in_order) {
                if (std::get<2>(index)) {
                    return false;
                }
            }

            return dynamic_cast<structure_builder::function_address*>(
                std::get<1>(instruction.operands_in_order[1])
            ) != nullptr;
        }

        return false;
    }
};

#endif