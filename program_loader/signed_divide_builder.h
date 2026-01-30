#ifndef SDIV_BUILDER_H
#define SDIV_BUILDER_H

#include "pch.h"
#include "complex_arithmetic_instruction_builder.h"

class signed_divide_builder : public complex_arithmetic_instruction_builder {
    void load_sign_extended_value() {
        this->zero_rax();

        this->move_r8_to_rbx();
        if (this->get_active_type() == 0b00) {
            this->write_bytes('\x66'); //movsx ax, [rbx]
            this->write_bytes('\x0f');
            this->write_bytes('\xbe');
            this->write_bytes('\x03');
        }
        else {
            this->use_r8_on_reg('\x8b', true, this->get_active_type());

            switch (this->get_active_type())
            {
            case 0b01: {
                this->write_bytes('\x66'); //cwd
                this->write_bytes('\x99');

                break;
            }

            case 0b10: {
                this->write_bytes('\x99'); //cdq
                break;
            }

            case 0b11: {
                this->write_bytes('\x48'); //cqo
                this->write_bytes('\x99');

                break;
            }

            default: break;
            }
        }
    }

public:
    using instruction_builder::visit;

    template<typename... args>
    signed_divide_builder(
        args&&... instruction_builder_args
    )
        :complex_arithmetic_instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() == 2, "This instruction must have only two arguments.");
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        if (this->get_argument_index() == 0) {
            this->load_variable_address(variable->get_id(), true);

            this->set_active_type(variable->get_active_type());
            this->load_sign_extended_value();
        }
        else {
            this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', true, variable.get());
        }
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        if (this->get_argument_index() == 0) {
            this->set_active_type(pointer->get_active_type());
            this->load_sign_extended_value();
        }
        else {
            this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type(), true);
        }
    }

    void build() override {
        this->self_call_next();
        this->self_call_next();

        this->check_if_r8_is_zero();
        this->use_r8_on_reg_with_two_opcodes(this->get_code_front(), this->get_code(1), false, this->get_active_type(), false, this->get_code_back());
        this->store_value_from_rax_to_rbx();
    }
};

#endif // !SDIV_BUILDER_H
