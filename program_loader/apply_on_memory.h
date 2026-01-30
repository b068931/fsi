#ifndef INC_DEC_BUILDER_H
#define INC_DEC_BUILDER_H

#include "pch.h"
#include "machine_codes_instruction_builder.h"

class apply_on_memory : public machine_codes_instruction_builder {
public:
    template<typename... args>
    apply_on_memory(
        args&&... instruction_builder_args
    )
        :machine_codes_instruction_builder{ std::forward<args>(instruction_builder_args)... }
    {
        this->assert_statement(this->get_arguments_count() >= 1, "This instruction requires at least one argument.");
    }

    void visit(std::unique_ptr<regular_variable> variable) override {
        this->assert_statement(variable->is_valid_active_type(), "Variable has an incorrect active type.", variable->get_id());
        this->create_variable_instruction(
            this->get_code(variable->get_active_type()),
            false,
            variable.get(),
            static_cast<std::uint8_t>(this->get_code_back())
        );
    }

    void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->accumulate_pointer_offset(pointer.get());
        this->add_base_address_to_pointer_dereference(pointer.get());

        std::uint8_t rex = 0b01000001;
        switch (pointer->get_active_type()) {
        case 0b01: {
            this->write_bytes<char>('\x66');
            break;
        }

        case 0b11: {
            rex |= 0b01001000;
            break;
        }

        default: break;
        }

        this->write_bytes<std::uint8_t>(rex); //inc/dec {one_byte/two_bytes/four_bytes/eight_bytes} [r8]
        this->write_bytes<char>(this->get_code(pointer->get_active_type()));

        this->write_bytes<char>(this->get_code_back() << 3 & 0b00111000); //r/m
    }

    void build() override {
        while (this->get_arguments_count() != 0) {
            this->self_call_next();
        }
    }
};

#endif // !INC_DEC_BUILDER_H
