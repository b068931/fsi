#ifndef BINARY_INSTRUCTION_FILTER_H
#define BINARY_INSTRUCTION_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"

struct binary_instruction {
    static constexpr translator_error_type error_message{ translator_error_type::binary_instruction };
    static bool check(const structure_builder::instruction& instruction) {
        return instruction.operands_in_order.size() == 2;
    }
};

#endif
