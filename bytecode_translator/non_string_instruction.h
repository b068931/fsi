#ifndef NON_STRING_INSTRUCTION_FILTER_H
#define NON_STRING_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct non_string_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::non_string_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.strings.empty();
    }
};

#endif
