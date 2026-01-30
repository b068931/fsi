#ifndef VARIABLE_IMMEDIATE_H
#define VARIABLE_IMMEDIATE_H

#include "variable.h"

template<typename T>
class variable_imm : public variable {
    T value;

public:
    variable_imm(T immediate_value)
        :variable{},
        value{ immediate_value }
    {}

    T get_value() const { return this->value; }
    void visit(instruction_builder* builder) override;
};

// Create declarations for specific types.
template<>
void variable_imm<std::uint8_t>::visit(instruction_builder* builder);

template<>
void variable_imm<std::uint16_t>::visit(instruction_builder* builder);

template<>
void variable_imm<std::uint32_t>::visit(instruction_builder* builder);

template<>
void variable_imm<std::uint64_t>::visit(instruction_builder* builder);

// Notify the compiler about the explicit instantiations elsewhere.
extern template void variable_imm<std::uint8_t>::visit(instruction_builder* builder);
extern template void variable_imm<std::uint16_t>::visit(instruction_builder* builder);
extern template void variable_imm<std::uint32_t>::visit(instruction_builder* builder);
extern template void variable_imm<std::uint64_t>::visit(instruction_builder* builder);

#endif // !VARIABLE_IMMEDIATE_H
