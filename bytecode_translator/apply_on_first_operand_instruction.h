#ifndef APPLY_ON_FIRST_OPERAND_INSTRUCTION_FILTER_H
#define APPLY_ON_FIRST_OPERAND_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct apply_on_first_operand_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::apply_on_first_operand_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        if (!instruction.operands_in_order.empty()) {
            return typeid(*std::get<1>(instruction.operands_in_order[0])) !=
                typeid(structure_builder::immediate_variable) || instruction.instruction_type == source_file_token::compare_instruction_keyword;
        }

        return false;
    }
};

#endif