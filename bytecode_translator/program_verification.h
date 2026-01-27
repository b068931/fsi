#ifndef FSI_PROGRAM_VERIFICATION_H
#define FSI_PROGRAM_VERIFICATION_H

#include <cstdint>
#include <limits>

#include "structure_builder.h"
#include "function_call_instruction.h"

constexpr auto max_functions_count = std::numeric_limits<std::uint32_t>::max();
constexpr auto max_instructions_count = std::numeric_limits<std::uint32_t>::max();
constexpr auto max_name_length = std::numeric_limits<std::uint8_t>::max();
constexpr auto max_function_arguments_count = std::numeric_limits<std::uint8_t>::max();

inline bool check_instructions_arguments(const structure_builder::file& file) {
    for (const structure_builder::function& func : file.functions) {
        for (const structure_builder::instruction& inst : func.body) {
            if (inst.operands_in_order.size() > max_instruction_arguments_count) {
                return false;
            }
        }
    }

    return true;
}
inline bool check_functions_count(const structure_builder::file& file) { return file.functions.size() <= max_functions_count; }
inline bool check_functions_size(const structure_builder::file& file) { 
    for (const structure_builder::function& func : file.functions) {
        if (func.body.size() > max_instructions_count) {
            return false;
        }
    }

    return true;
}
inline std::vector<std::string> check_functions_bodies(const structure_builder::file& file) {
    std::vector<std::string> result{};
    for (const structure_builder::function& function : file.functions) {
        if (function.body.empty()) {
            result.push_back(function.name);
        }
    }

    return result;
}



#endif
