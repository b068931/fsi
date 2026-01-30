#ifndef APPLY_RIGHT_HAND_BITS_ON_LEFT_HAND_BINARY_H
#define APPLY_RIGHT_HAND_BITS_ON_LEFT_HAND_BINARY_H

#include "pch.h"
#include "arithmetic_instruction_builder.h"

class apply_right_hand_bits_on_left_hand_binary : public arithmetic_instruction_builder {
public:
    using instruction_builder::visit;

    template<typename... args>
    apply_right_hand_bits_on_left_hand_binary(
        args&&... instruction_builder_args
    )
        :arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() == 2, "This instruction must have only two arguments.");
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
        if (this->get_argument_index() == 0) {
            this->load_variable_address(variable->get_id());
            this->set_active_type(variable->get_active_type());
        }
        else { //mov r8, [rbp+disp32]
            this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
        }
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        if (this->get_argument_index() == 0) {
            this->use_r8_on_reg('\x8b', false, 0b11); //mov rax, r8
            this->set_active_type(pointer->get_active_type());
        }
        else { //mov r8, [r8]
            this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
        }
    }

    void build() override {
        this->zero_rax(); //rax contains destination address for a value

        this->self_call_next();
        this->self_call_next();

        std::uint8_t rex = '\x44';

        switch (this->get_active_type()) {
        case 0b01: {
            this->write_bytes('\x66');
            break;
        }

        case 0b11: {
            rex |= 0b00001000;
            break;
        }

        default: break;
        }

        this->write_bytes(rex);

        this->write_bytes(this->get_code(this->get_active_type()));
        this->write_bytes('\x00');
    }
};

#endif // !APPLY_RIGHT_HAND_BITS_ON_LEFT_HAND_BINARY_H
