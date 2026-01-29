#ifndef FUNCTION_CALL_BUILDER_H
#define FUNCTION_CALL_BUILDER_H

#include "pch.h"
#include "general_function_call_bulider.h"

class function_call_builder : public general_function_call_builder {
    std::int32_t stack_address;
    std::uint32_t stack_allocation_size;

    std::uint64_t total_arguments_processed;

    entity_id this_function_id;
    std::uint32_t function_jump_table_index;
    const runs_container::function_signature* fnc_signature;

    void prologue() {
        ++this->total_arguments_processed;
        std::uint8_t translated_type = this->translate_current_type_to_memory_layouts_builder_type();

        this->assert_statement(
            this->get_argument_index() <= this->fnc_signature->argument_types.size(), 
            std::format(
                "Too many arguments passed to function body. Expected {}.",
                this->fnc_signature->argument_types.size()
            ),
            this->this_function_id
        );

        this->assert_statement(
            this->fnc_signature->argument_types[this->get_argument_index() - 1].second == translated_type &&
            (this->get_current_variable_type() & 0b1100) != 0,
            "Function call with incorrect arguments.",
            this->this_function_id
        );

        memory_layouts_builder::variable_size size = memory_layouts_builder::get_variable_size(translated_type);
        if (size != memory_layouts_builder::variable_size::UNKNOWN) {
            this->stack_address -= static_cast<std::uint8_t>(size);
        }
    }

    template<typename T>
    void place_immediate(T value) {
        this->prologue();
        this->general_function_call_builder::place_immediate(value, this->stack_address, 0b101);
    }
    void move_value_from_reg000_to_stack(std::uint8_t active_type, bool is_R) {
        this->move_value_from_reg000_to_memory(active_type, is_R, this->stack_address, 0b101);
    }
public:
    template<typename... args>
    function_call_builder(
        args&&... instruction_builder_args
    )
        :general_function_call_builder{ std::forward<args>(instruction_builder_args)... },
        stack_address{ 0 },
        stack_allocation_size{ 0 },
        total_arguments_processed{ 0 },
        this_function_id{ 0 },
        function_jump_table_index{},
        fnc_signature{ nullptr }
    {}

    virtual void visit(std::unique_ptr<function> fnc) override {
        if (!this->fnc_signature) {
            this->function_jump_table_index = static_cast<std::uint32_t>(this->get_function_table_index(fnc->get_id()));

            auto found_signature = this->get_file_info().function_signatures.find(fnc->get_id());
            this->assert_statement(
                found_signature != this->get_file_info().function_signatures.end(), 
                "Unknown function.", 
                fnc->get_id()
            );

            this->this_function_id = fnc->get_id();
            this->fnc_signature = &found_signature->second;
        }
        else {
            variable_imm immediate_function_displacement{
                (this->get_function_table_index(fnc->get_id()))
            };

            this->place_immediate(&immediate_function_displacement);
        }
    }
    virtual void visit(std::unique_ptr<variable_imm<std::uint8_t>> value) override {
        this->place_immediate(value.get());
    }
    virtual void visit(std::unique_ptr<variable_imm<std::uint16_t>> value) override {
        this->place_immediate(value.get());
    }
    virtual void visit(std::unique_ptr<variable_imm<std::uint32_t>> value) override {
        this->place_immediate(value.get());
    }
    virtual void visit(std::unique_ptr<variable_imm<std::uint64_t>> value) override {
        this->place_immediate(value.get());
    }
    virtual void visit(std::unique_ptr<regular_variable> variable) override {
        this->prologue();

        this->create_variable_instruction_with_two_opcodes('\x8a', '\x8b', false, variable.get(), 0, this->stack_allocation_size);
        this->move_value_from_reg000_to_stack(variable->get_active_type(), false);
    }
    virtual void visit(std::unique_ptr<pointer> pointer) override {
        this->prologue();
        this->load_pointer_info(
            this->get_variable_info(pointer->get_id()), 
            this->stack_allocation_size
        );

        this->write_bytes('\x49'); //mov rax, r15
        this->write_bytes('\x8b');
        this->write_bytes('\xc7');

        this->move_value_from_reg000_to_stack(0b11, false);
    }
    virtual void visit(std::unique_ptr<dereferenced_pointer> pointer) override {
        this->prologue();

        this->accumulate_pointer_offset(pointer.get(), this->stack_allocation_size);
        this->add_base_address_to_pointer_dereference(pointer.get(), this->stack_allocation_size);

        this->use_r8_on_reg_with_two_opcodes('\x8a', '\x8b', true, pointer->get_active_type());
        this->move_value_from_reg000_to_stack(pointer->get_active_type(), false);
    }

    virtual void build() override {
        this->self_call_by_type(0b1110);
        std::uint32_t stack_to_allocate = 0;
        for (const auto& fnc_argument : this->fnc_signature->argument_types) {
            memory_layouts_builder::variable_size arg_size =
                memory_layouts_builder::get_variable_size(fnc_argument.second);

            this->assert_statement(
                arg_size != memory_layouts_builder::variable_size::UNKNOWN,
                std::format("Encountered an argument with an unknown size."),
                fnc_argument.first
            );

            stack_to_allocate += static_cast<std::uint8_t>(arg_size);
        }

        this->stack_allocation_size = stack_to_allocate;
        this->generate_stack_allocation_code(stack_to_allocate);
        while (this->get_arguments_count() != 0) {
            this->self_call_next();
        }

        this->assert_statement(
            this->total_arguments_processed == this->fnc_signature->argument_types.size(),
            std::format(
                "Incorrect number of arguments passed to function body. Expected {}, got {}.",
                this->fnc_signature->argument_types.size(),
                this->total_arguments_processed
            ),
            this->this_function_id
        );

        this->write_bytes('\x41'); //call [r11 + disp32]
        this->write_bytes('\xff');
        this->write_bytes('\x93');
        this->write_bytes(this->function_jump_table_index);
    }
};

#endif // !FUNCTION_CALL_BUILDER_H
