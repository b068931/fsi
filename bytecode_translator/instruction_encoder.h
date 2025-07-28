#ifndef FSI_INSTRUCTION_ENCODER_H
#define FSI_INSTRUCTION_ENCODER_H

#include <vector>
#include <cstdint>

#include "structure_builder.h"
#include "source_file_token.h"

class instruction_encoder : public structure_builder::variable_visitor {
private:
    std::vector<char> instruction_symbols;
    std::size_t position_in_type_bytes{2};

    instruction_encoder() = default;
    static std::uint8_t convert_type_to_uint8(source_file_token token_type) {
        std::uint8_t type = 0;
        switch (token_type) {  // NOLINT(clang-diagnostic-switch-enum)
            case source_file_token::one_byte_type_keyword: {
                type = 0;
                break;
            }
            case source_file_token::two_bytes_type_keyword: {
                type = 1;
                break;
            }
            case source_file_token::four_bytes_type_keyword: {
                type = 2;
                break;
            }
            case source_file_token::eight_bytes_type_keyword: {
                type = 3;
                break;
            }
            case source_file_token::no_return_module_call_keyword: {
                type = 1;
                break;
            }
            case source_file_token::memory_type_keyword: {
                type = 3;
                break;
            }
            default: {
                assert(false && "you should not see this");
                break;
            }
        }

        return type;
    }

    template<typename type>
    void write_bytes(type value) {
        char* bytes = reinterpret_cast<char*>(&value);
        for (std::size_t counter = 0; counter < sizeof(value); ++counter) {
            this->instruction_symbols.push_back(bytes[counter]);
        }
    }
    void write_id(structure_builder::entity_id id) {
        this->write_bytes<std::uint64_t>(static_cast<std::uint64_t>(id));
    }
    void encode_active_type(std::uint8_t active_type, std::uint8_t type_mod) {
        std::uint8_t type_bits =
            (type_mod & 0b11) << 2 | 0b11 & active_type;
        std::size_t byte_index = this->position_in_type_bytes / 2 - 1 + 2;
        if (this->position_in_type_bytes % 2 == 0) {
            type_bits <<= 4;
        }

        ++this->position_in_type_bytes;
        this->instruction_symbols[byte_index] |= type_bits;
    }
public:
    virtual void visit(source_file_token active_type, const structure_builder::pointer_dereference* variable, bool is_signed) override {
        this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b10);
        this->write_id(variable->pointer_variable->id);

        this->instruction_symbols.push_back(static_cast<char>(variable->derefernce_indexes.size()));
        for (structure_builder::regular_variable* var : variable->derefernce_indexes) {
            this->write_id(var->id);
        }
    }
    virtual void visit(source_file_token active_type, const structure_builder::regular_variable* variable, bool is_signed) override {
        if (is_signed) {
            this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b00);
        }
        else {
            if (active_type == source_file_token::no_return_module_call_keyword) {
                this->encode_active_type(0b01, 0b11);
            }
            else if (active_type == source_file_token::memory_type_keyword) {
                this->encode_active_type(0b11, 0b11);
            }
            else {
                this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b10);
            }
        }

        this->write_id(variable->id);
    }
    virtual void visit(source_file_token active_type, const structure_builder::immediate_variable* variable, bool is_signed) override {
        this->encode_active_type(instruction_encoder::convert_type_to_uint8(active_type), 0b01);
        switch (variable->type) {
            case source_file_token::one_byte_type_keyword: {
                this->write_bytes<std::uint8_t>(static_cast<std::uint8_t>(variable->imm_val));
                break;
            }
            case source_file_token::two_bytes_type_keyword: {
                this->write_bytes<std::uint16_t>(static_cast<std::uint16_t>(variable->imm_val));
                break;
            }
            case source_file_token::four_bytes_type_keyword: {
                this->write_bytes<std::uint32_t>(static_cast<std::uint32_t>(variable->imm_val));
                break;
            }
            case source_file_token::eight_bytes_type_keyword: {
                this->write_bytes<std::uint64_t>(static_cast<std::uint64_t>(variable->imm_val));
                break;
            }
            default: break;
        }
    }
    virtual void visit(source_file_token active_type, const structure_builder::function_address* variable, bool is_signed) override {
        this->encode_active_type(2, 0b11);
        this->write_id(variable->func->id);
    }
    virtual void visit(source_file_token active_type, const structure_builder::module_variable* variable, bool is_signed) override {
        this->encode_active_type(0, 0b11);
        this->write_id(variable->mod->id);
    }
    virtual void visit(source_file_token active_type, const structure_builder::module_function_variable* variable, bool is_signed) override {
        this->encode_active_type(2, 0b11);
        this->write_id(variable->func->id);
    }
    virtual void visit(source_file_token active_type, const structure_builder::jump_point_variable* variable, bool is_signed) override {
        this->encode_active_type(1, 0b11);
        this->write_id(variable->point->id);
    }
    virtual void visit(source_file_token active_type, const structure_builder::string_constant* variable, bool is_signed) override {
        this->encode_active_type(1, 0b11);
        this->write_id(variable->value->id);
    }

    static std::vector<char> encode_instruction(const structure_builder::instruction& current_instruction, const std::map<source_file_token, std::uint8_t>& operation_codes) {
        instruction_encoder self{};
        bool is_odd = current_instruction.operands_in_order.size() % 2;

        auto found_opcode = operation_codes.find(current_instruction.instruction_type);
        if (found_opcode != operation_codes.end()) {
            self.instruction_symbols.push_back(static_cast<std::uint8_t>(current_instruction.operands_in_order.size()) << 4); //higher four bits: arguments count, lower four bits: additional type bits if argument count is odd
            self.instruction_symbols.push_back(found_opcode->second);
            self.instruction_symbols.resize(self.instruction_symbols.size() + is_odd + current_instruction.operands_in_order.size() / 2);

            for (const auto& var : current_instruction.operands_in_order) {
                structure_builder::variable* variable_to_visit = std::get<1>(var);
                if (variable_to_visit) { //if variable_to_visit is nullptr then entity_id equals to 0
                    variable_to_visit->visit(
                        std::get<0>(var), &self, std::get<2>(var));
                }
                else {
                    self.encode_active_type(0b01, 0b11); //do not return from module call
                    self.write_id(0);
                }
            }

            if (current_instruction.operands_in_order.size() % 2 == 1) { //if number of arguments is odd
                std::size_t additional_type_bits_index = 2 + current_instruction.operands_in_order.size() / 2; //move half filled type bits to prefix byte
                self.instruction_symbols[0] |= self.instruction_symbols[additional_type_bits_index] >> 4 & 0b1111;

                self.instruction_symbols.erase(self.instruction_symbols.begin() + additional_type_bits_index);
            }
        }

        return self.instruction_symbols;
    }
};

#endif
