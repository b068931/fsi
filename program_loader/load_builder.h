#ifndef LOAD_BUILDER_H
#define LOAD_BUILDER_H

#include "instruction_builder.h"

class load_builder : public instruction_builder {
    void check_saved_variable_type(std::uint8_t type) {
        this->write_bytes('\x41'); //cmp byte ptr [r9 + 8], type
        this->write_bytes('\x80');
        this->write_bytes('\x79');
        this->write_bytes('\x08');
        this->write_bytes(type);

        this->write_bytes('\x74'); //je end
        this->write_bytes(static_cast<std::uint8_t>(program_termination_code_size));

        this->generate_program_termination_code(program_loader::termination_codes::incorrect_saved_variable_type);
        //:end
    }
    void move_saved_variable_value_to_rax() {
        this->write_bytes('\x49');
        this->write_bytes('\x8b');
        this->write_bytes('\x01');
    }

public:
    template<typename... args>
    load_builder(
        const std::vector<char>* machine_codes,
        args&&... instruction_builder_args
    )
        :instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() == 1 && !machine_codes, "This instruction must have only one argument.");
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
        this->check_saved_variable_type(variable->get_active_type());
        this->load_variable_address(variable->get_id(), true);

        this->use_r8_on_reg_with_two_opcodes('\x88', '\x89', true, variable->get_active_type());
    }

    void visit(std::unique_ptr<pointer> pointer) override {
        this->check_saved_variable_type(4);
        this->load_variable_address(pointer->get_id(), true);

        this->use_r8_on_reg_with_two_opcodes('\x88', '\x89', true, 0b11);
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->check_saved_variable_type(pointer->get_active_type());

        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        this->use_r8_on_reg_with_two_opcodes('\x88', '\x89', true, pointer->get_active_type());
    }

    void build() override {
        this->move_saved_variable_value_to_rax();
        this->self_call_next();
    }
};

#endif // !LOAD_BUILDER_H
