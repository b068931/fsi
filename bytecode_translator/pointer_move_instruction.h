#ifndef POINTER_MOVE_INSTRUCTION_FILTER_H
#define POINTER_MOVE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct pointer_move_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::pointer_ref_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        if (instruction.immediates.empty()) {
            for (const auto& index : instruction.operands_in_order) {
                structure_builder::regular_variable* current_argument =
                    dynamic_cast<structure_builder::regular_variable*>(std::get<1>(index));

                if (current_argument && current_argument->type != source_file_token::memory_type_keyword) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }
};

#endif
