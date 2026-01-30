#ifndef CONVERT_TO_JUMP_TABLE_DISPLACEMENT_H
#define CONVERT_TO_JUMP_TABLE_DISPLACEMENT_H

#include "instruction_builder.h"

class get_function_address : public instruction_builder {
    std::uint32_t function_displacement;

public:
    template<typename... args>
    get_function_address(
        const std::vector<char>* machine_codes,
        args&&... instruction_builder_args
    )
        :instruction_builder{ std::forward<args>(instruction_builder_args)... },
        function_displacement{ 0 }
    {
        this->assert_statement(this->get_arguments_count() == 2 && !machine_codes, "This instruction must have only two arguments.");
    }

    void visit(std::unique_ptr<function> fnc) override {
        this->function_displacement =
            static_cast<std::uint32_t>(this->get_function_table_index(fnc->get_id()));
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        this->assert_statement(
            variable->is_valid_active_type() && 
            variable->get_active_type() == 0b11 &&
            this->get_argument_index() == 0,
            "Regular variable or pointer dereference must be used as the first argument. Use 'eight_bytes' as the active type."
        );

        this->load_variable_address(variable->get_id());
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->assert_statement(
            pointer->get_active_type() == 0b11 &&
            this->get_argument_index() == 0,
            "Regular variable or pointer dereference must be used as the first argument. Use 'eight_bytes' as the active type."
        );

        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        this->use_r8_on_reg('\x8b', false, 0b11); //mov rax, r8
    }

    void build() override {
            this->self_call_next();
            this->self_call_next();

            this->write_bytes('\x48');
            this->write_bytes('\xc7');
            this->write_bytes('\x00');
            this->write_bytes(this->function_displacement);
    }
};

#endif
