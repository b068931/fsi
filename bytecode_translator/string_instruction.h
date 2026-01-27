#ifndef STRING_INSTRUCTION_FILTER_H
#define STRING_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct string_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::string_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        if (instruction.strings.size() == 1 && instruction.operands_in_order.size() >= 2) {
            return std::get<0>(instruction.operands_in_order[0]) == source_file_token::memory_type_keyword &&
                std::get<0>(instruction.operands_in_order[1]) == source_file_token::string_argument_keyword;
        }

        return false;
    }
};

#endif
