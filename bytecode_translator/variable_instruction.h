#ifndef VARIABLE_INSTRUCTION_FILTER_H
#define VARIABLE_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct variable_instruction { //var instructions can only dereference pointers and can not use them as pure arguments
    static constexpr translator_error_type error_message{ translator_error_type::var_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        for (const auto& index : instruction.operands_in_order) {
            structure_builder::regular_variable* current_argument = 
                dynamic_cast<structure_builder::regular_variable*>(std::get<1>(index));
            
            if (current_argument && current_argument->type == source_file_token::memory_type_keyword) {
                return false;
            }
        }

        return true;
    }
};

#endif