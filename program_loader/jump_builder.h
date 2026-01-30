#ifndef JMP_BUILDER_H
#define JMP_BUILDER_H

#include "pch.h"
#include "machine_codes_instruction_builder.h"

class jump_builder : public machine_codes_instruction_builder {
public:
    template<typename... args>
    jump_builder(
        args&&... instruction_builder_args
    )
        :machine_codes_instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() == 1, "This instruction must have only one argument.");
    }

    void visit(std::unique_ptr<specialized_variable> jump_point) override {
        this->write_bytes('\x41');
        this->write_bytes(this->get_code_front());
        this->write_bytes<char>('\x83' | this->get_code_back() << 3 & 0b00111000);

        std::uint32_t index = static_cast<std::uint32_t>(this->get_jump_point_table_index(jump_point->get_id()));
        this->write_bytes(index);
    }

    void build() override {
        this->self_call_next();
    }
};

#endif // !JMP_BUILDER_H
