#ifndef GENERAL_INSTRUCTION_FILTER_H
#define GENERAL_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct general_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::general_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        //at first, we make sure that this instruction won't use arguments specific to function calls
        bool function_arguments = instruction.func_addresses.empty() && instruction.modules.empty() && instruction.module_functions.empty();
        if (function_arguments) {
            for (const auto& index : instruction.operands_in_order) {
                if (std::get<2>(index)) { //now we make sure that this instruction won't use signed variables
                    return false;
                }
            }
            
            return true;
        }

        return false;
    }
};

#endif
