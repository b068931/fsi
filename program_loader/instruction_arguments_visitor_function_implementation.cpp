#include "pch.h"
#include "instruction_arguments_classes.h"
#include "instruction_builder.h"

template<>
void variable_imm<std::uint8_t>::visit(instruction_builder* builder) {
	builder->visit(std::unique_ptr<variable_imm<std::uint8_t>>{ this });
}

template<>
void variable_imm<std::uint16_t>::visit(instruction_builder* builder) {
	builder->visit(std::unique_ptr<variable_imm<std::uint16_t>>{ this });
}

template<>
void variable_imm<std::uint32_t>::visit(instruction_builder* builder) {
	builder->visit(std::unique_ptr<variable_imm<std::uint32_t>>{ this });
}

template<>
void variable_imm<std::uint64_t>::visit(instruction_builder* builder) {
	builder->visit(std::unique_ptr<variable_imm<std::uint64_t>>{ this });
}

void function::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<function>{ this }); }
void engine_module::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<engine_module>{ this }); }
void pointer::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<pointer>{ this }); }
void specialized_variable::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<specialized_variable>{ this }); }
void dereferenced_pointer::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<dereferenced_pointer>{ this }); }
void signed_variable::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<signed_variable>{ this }); }
void regular_variable::visit(instruction_builder* builder) { builder->visit(std::unique_ptr<regular_variable>{ this }); }
