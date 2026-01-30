#ifndef DIV_BUILDER_H
#define DIV_BUILDER_H

#include "pch.h"
#include "complex_arithmetic_instruction_builder.h"

class divide_builder : public complex_arithmetic_instruction_builder {
public:
    using instruction_builder::visit;

    template<typename... args>
    divide_builder(
        args&&... instruction_builder_args
    )
        :complex_arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() == 2, "This instruction must have only two arguments.");
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        if (this->get_argument_index() == 0) {
            this->load_variable_address(variable->get_id(), true);
            this->move_r8_to_rbx();

            this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', false, variable.get());
            this->set_active_type(variable->get_active_type());
        }
        else {
            this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
        }
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        if (this->get_argument_index() == 0) {
            this->move_r8_to_rbx();
            this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type());

            this->set_active_type(pointer->get_active_type());
        }
        else {
            this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
        }
    }

    void build() override {
        this->self_call_next();
        this->self_call_next();

        this->check_if_r8_is_zero();
        this->zero_rdx();

        this->use_r8_on_reg_with_two_opcodes(this->get_code_front(), this->get_code(1), false, this->get_active_type(), false, this->get_code_back());
        this->store_value_from_rax_to_rbx();

        this->save_type_to_program_stack(this->get_active_type());
        if (this->get_active_type() == 0b00) {
            this->write_bytes('\x88'); //mov al, ah - two instructions because you can not use ah with rex prefix
            this->write_bytes('\xe0');

            this->write_bytes('\x41'); //mov [r9], al
            this->write_bytes('\x88');
            this->write_bytes('\x01');
        }
        else {
            this->write_bytes('\x49'); //mov [r9], rdx
            this->write_bytes('\x89');
            this->write_bytes('\x11');
        }
    }
};

#endif // !DIV_BUILDER_H
