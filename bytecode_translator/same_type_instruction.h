#ifndef SAME_TYPE_INSTRUCTION_FILTER_H
#define SAME_TYPE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct same_type_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::same_type_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        if (instruction.operands_in_order.size() >= 2) {
            source_file_token main_active_type = std::get<0>(instruction.operands_in_order[0]);
            for (std::size_t index = 1, count = instruction.operands_in_order.size(); index < count; ++index) {
                if (main_active_type != std::get<0>(instruction.operands_in_order[index])) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }
};

#endif
