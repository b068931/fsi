#ifndef FSI_BYTECODE_TRANSLATOR_H
#define FSI_BYTECODE_TRANSLATOR_H

#include <algorithm>
#include <vector>
#include <cstdint>
#include <iostream>

#include "apply_on_first_operand_instruction.h"
#include "binary_instruction.h"
#include "create_filter.h"
#include "data_instruction.h"
#include "different_type_instruction.h"
#include "different_type_multi_instruction.h"
#include "function_call_instruction.h"
#include "general_instruction.h"
#include "get_function_address_instruction.h"
#include "jump_instruction.h"
#include "load_variable_state_instruction.h"
#include "module_function_call_instruction.h"
#include "multi_instruction.h"
#include "non_string_instruction.h"
#include "pointer_move_instruction.h"
#include "program_function_call_instruction.h"
#include "same_type_instruction.h"
#include "save_variable_state_instruction.h"
#include "string_instruction.h"
#include "variable_instruction.h"

#include "structure_builder.h"
#include "translator_error_type.h"
#include "instruction_checking.h"
#include "instruction_encoder.h"
#include "program_verification.h"

//This class will generate logic errors. Programs with logic errors can be translated to byte code, but they won't be compiled to machine code
class bytecode_translator {
public:
    using entity_id = structure_builder::entity_id;
private:
    void check_logic_errors(const std::vector<instruction_check*>& filters_list, const structure_builder::instruction& current_instruction) {
        for (instruction_check* check : filters_list) {
            if (check->is_match(current_instruction.instruction_type)) {
                check->check_errors(current_instruction, this->logic_errors);
                return;
            }
        }

        this->logic_errors.push_back(translator_error_type::unknown_instruction);
    }

    static constexpr std::uint8_t modules_run = 0;
    static constexpr std::uint8_t jump_points_run = 1;
    static constexpr std::uint8_t function_signatures_run = 2;
    static constexpr std::uint8_t function_body_run = 3;
    static constexpr std::uint8_t program_exposed_functions_run = 4;
    static constexpr std::uint8_t program_strings_run = 5;
    static constexpr std::uint8_t program_debug_run = 6;

    structure_builder::file* file_structure;
    std::ostream* out_stream;

    std::vector<translator_error_type> logic_errors;
    void add_new_logic_error(translator_error_type err) { this->logic_errors.push_back(err); }

    template<typename type>
    void write_bytes(type value) {
        this->out_stream->write(reinterpret_cast<char*>(&value), sizeof(value));
    }

    void write_n_bytes(const char* bytes, std::size_t size) {
        this->out_stream->write(bytes, static_cast<std::streamsize>(size));
    }
    void write_8_bytes(std::uint64_t value) {
        this->write_bytes<std::uint64_t>(value);
    }
    void write_4_bytes(std::uint32_t value) {
        this->write_bytes<std::uint32_t>(value);
    }
    void write_2_bytes(std::uint16_t value) {
        this->write_bytes<std::uint16_t>(value);
    }
    void write_1_byte(std::uint8_t value) {
        this->write_bytes<std::uint8_t>(value);
    }

    static std::uint8_t convert_type_to_uint8(source_file_token token_type) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch-default"

        std::uint8_t type = 0;
        switch (token_type) {  // NOLINT(clang-diagnostic-switch-enum)
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
            case source_file_token::memory_type_keyword: {
                type = 4;
                break;
            }
            default: break;
        }

#pragma clang diagnostic pop

        return type;
    }
    static std::map<source_file_token, std::uint8_t> get_operation_codes() {
        return {
            {source_file_token::subtract_instruction_keyword, 0},
            {source_file_token::signed_subtract_instruction_keyword, 1},
            {source_file_token::divide_instruction_keyword, 2},
            {source_file_token::signed_divide_instruction_keyword, 3},
            {source_file_token::compare_instruction_keyword, 4},
            {source_file_token::move_instruction_keyword, 5},
            {source_file_token::add_instruction_keyword, 6},
            {source_file_token::signed_add_instruction_keyword, 7},
            {source_file_token::multiply_instruction_keyword, 8},
            {source_file_token::signed_multiply_instruction_keyword, 9},
            {source_file_token::increment_instruction_keyword, 10},
            {source_file_token::decrement_instruction_keyword, 11},
            {source_file_token::bit_xor_instruction_keyword, 12},
            {source_file_token::bit_and_instruction_keyword, 13},
            {source_file_token::bit_or_instruction_keyword, 14},
            {source_file_token::jump_instruction_keyword, 15},
            {source_file_token::jump_equal_instruction_keyword, 16},
            {source_file_token::jump_not_equal_instruction_keyword, 17},
            {source_file_token::jump_greater_instruction_keyword, 18},
            {source_file_token::jump_greater_equal_instruction_keyword, 19},
            {source_file_token::jump_less_equal_instruction_keyword, 20},
            {source_file_token::jump_less_instruction_keyword, 27},
            {source_file_token::jump_above_instruction_keyword, 21},
            {source_file_token::jump_above_equal_instruction_keyword, 22},
            {source_file_token::jump_below_instruction_keyword, 23},
            {source_file_token::jump_below_equal_instruction_keyword, 24},
            {source_file_token::function_call, 25},
            {source_file_token::module_call, 26},
            {source_file_token::bit_not_instruction_keyword, 28},
            {source_file_token::save_value_instruction_keyword, 29},
            {source_file_token::load_value_instruction_keyword, 30},
            {source_file_token::move_pointer_instruction_keyword, 31},
            {source_file_token::bit_shift_left_instruction_keyword, 32},
            {source_file_token::bit_shift_right_instruction_keyword, 33},
            {source_file_token::get_function_address_instruction_keyword, 34},
            {source_file_token::copy_string_instruction_keyword, 35}
        };
    }
    static std::vector<instruction_check*> get_instruction_filters() {
        using binary_instruction_filter = create_filter<
            general_instruction, data_instruction,
            variable_instruction, apply_on_first_operand_instruction,
            same_type_instruction, binary_instruction,
            non_string_instruction>::type;

        using multi_instruction_filter = create_filter<
            general_instruction, data_instruction,
            variable_instruction, apply_on_first_operand_instruction,
            same_type_instruction, multi_instruction,
            non_string_instruction>::type;

        using different_type_multi_instruction_filter = create_filter<
            general_instruction, data_instruction,
            variable_instruction, different_type_instruction,
            different_type_multi_instruction, non_string_instruction>::type;

        using jump_instruction_filter = create_filter<
            general_instruction, jump_instruction,
            non_string_instruction>::type;

        using save_variable_state_instruction_filter = create_filter<
            general_instruction, data_instruction,
            save_variable_state_instruction, non_string_instruction>::type;

        using load_variable_state_instruction_filter = create_filter<
            general_instruction, data_instruction,
            save_variable_state_instruction, load_variable_state_instruction,
            non_string_instruction>::type;

        using program_function_call_instruction_filter = create_filter<
            function_call_instruction, program_function_call_instruction,
            non_string_instruction>::type;

        using module_function_call_instruction_filter = create_filter<
            function_call_instruction, module_function_call_instruction,
            non_string_instruction>::type;

        using pointer_ref_instruction_filter = create_filter<
            general_instruction, data_instruction,
            binary_instruction, pointer_move_instruction,
            non_string_instruction>::type;

        using bit_shift_instruction_filter = create_filter<
            general_instruction, data_instruction,
            variable_instruction, apply_on_first_operand_instruction,
            binary_instruction, non_string_instruction>::type;

        using get_function_address_filter = create_filter<
            data_instruction, variable_instruction,
            different_type_instruction, binary_instruction,
            get_function_address_instruction, non_string_instruction>::type;

        using string_instruction_filter = create_filter<
            string_instruction, general_instruction,
            data_instruction, different_type_instruction,
            binary_instruction, string_instruction>::type;

        return {
            new generic_instruction_check<binary_instruction_filter>{{
                source_file_token::subtract_instruction_keyword,
                source_file_token::signed_subtract_instruction_keyword,
                source_file_token::divide_instruction_keyword,
                source_file_token::signed_divide_instruction_keyword,
                source_file_token::compare_instruction_keyword,
                source_file_token::move_instruction_keyword,
                source_file_token::bit_and_instruction_keyword,
                source_file_token::bit_or_instruction_keyword,
                source_file_token::bit_xor_instruction_keyword
            }},
            new generic_instruction_check<multi_instruction_filter>{{
                source_file_token::add_instruction_keyword,
                source_file_token::signed_add_instruction_keyword,
                source_file_token::multiply_instruction_keyword,
                source_file_token::signed_multiply_instruction_keyword
            }},
            new generic_instruction_check<different_type_multi_instruction_filter>{{
                source_file_token::increment_instruction_keyword,
                source_file_token::decrement_instruction_keyword,
                source_file_token::bit_not_instruction_keyword
            }},
            new generic_instruction_check<jump_instruction_filter>{{
                source_file_token::jump_instruction_keyword,
                source_file_token::jump_equal_instruction_keyword,
                source_file_token::jump_not_equal_instruction_keyword,
                source_file_token::jump_greater_instruction_keyword,
                source_file_token::jump_greater_equal_instruction_keyword,
                source_file_token::jump_less_instruction_keyword,
                source_file_token::jump_less_equal_instruction_keyword,
                source_file_token::jump_above_instruction_keyword,
                source_file_token::jump_above_equal_instruction_keyword,
                source_file_token::jump_below_instruction_keyword,
                source_file_token::jump_below_equal_instruction_keyword
            }},
            new generic_instruction_check<program_function_call_instruction_filter>{{
                source_file_token::function_call
            }},
            new generic_instruction_check<module_function_call_instruction_filter>{{
                source_file_token::module_call
            }},
            new generic_instruction_check<save_variable_state_instruction_filter>{{
                source_file_token::save_value_instruction_keyword
            }},
            new generic_instruction_check<load_variable_state_instruction_filter>{ {
                source_file_token::load_value_instruction_keyword
            }},
            new generic_instruction_check<pointer_ref_instruction_filter>{{
                source_file_token::move_pointer_instruction_keyword
            }},
            new generic_instruction_check<bit_shift_instruction_filter>{ {
                source_file_token::bit_shift_left_instruction_keyword,
                source_file_token::bit_shift_right_instruction_keyword
            }},
            new generic_instruction_check<get_function_address_filter>{{
                source_file_token::get_function_address_instruction_keyword
            }},
            new generic_instruction_check<string_instruction_filter>{{
                source_file_token::copy_string_instruction_keyword
            }}
        };
    }

    auto write_run_header(std::uint8_t run_type) {
        this->write_1_byte(run_type);
        auto saved_position = this->out_stream->tellp(); //we will return here later after we calculate this run's size
        
        this->write_8_bytes(0);
        return saved_position;
    }
    void write_run_footer(const std::streampos& saved_position, std::uint64_t run_size) {
        this->out_stream->seekp(saved_position); //go back and write this run's size
        this->write_8_bytes(run_size);

        this->out_stream->seekp(0, std::ios::end); //go back to the end of this file
    }

    void create_modules_run() { //8 bytes: modules count, 8 bytes: module's entity_id, 8 bytes: number of functions in this module, 1 byte: module name length, module name, 8 bytes: entity_id, 1 byte module function name length, module function name;
        std::uint64_t run_size = 8;
        auto saved_position = this->write_run_header(modules_run);

        this->write_8_bytes(this->file_structure->modules.size());
        for (const structure_builder::engine_module& mod : this->file_structure->modules) {
            this->write_8_bytes(mod.id);
            this->write_8_bytes(mod.functions_names.size());

            std::size_t module_name_size = mod.name.size();
            if (module_name_size > max_name_length) {
                this->add_new_logic_error(translator_error_type::name_too_long);
            }

            this->write_1_byte(static_cast<std::uint8_t>(mod.name.size()));
            this->write_n_bytes(mod.name.c_str(), mod.name.size());
            
            run_size += 17 + module_name_size;
            for (const structure_builder::module_function& mod_fnc : mod.functions_names) {
                this->write_8_bytes(mod_fnc.id);
                this->write_1_byte(static_cast<std::uint8_t>(mod_fnc.name.size()));

                std::size_t module_function_name_size = mod_fnc.name.size();
                if (module_function_name_size > max_name_length) {
                    this->add_new_logic_error(translator_error_type::name_too_long);
                }

                this->out_stream->write(mod_fnc.name.c_str(), static_cast<std::streamsize>(module_function_name_size));
                run_size += 9 + module_function_name_size;
            }
        }

        this->write_run_footer(saved_position, run_size);
    }
    void create_jump_points_run() { //4 bytes: function index, 4 bytes instruction index, 8 bytes: entity_id
        std::uint64_t run_size = 0;
        auto saved_position = this->write_run_header(jump_points_run);
        
        std::uint32_t function_index = 0;
        for (structure_builder::function& fnc : this->file_structure->functions) {
            for (const structure_builder::jump_point& jmp_point : fnc.jump_points) {
                if (jmp_point.index == std::numeric_limits<std::uint32_t>::max()) {
                    this->logic_errors.push_back(translator_error_type::unknown_jump_point);
                }

                this->write_4_bytes(function_index);
                this->write_4_bytes(jmp_point.index);
                this->write_8_bytes(jmp_point.id);

                run_size += 16; //8 + 4 + 4
            }

            ++function_index;
        }

        this->write_run_footer(saved_position, run_size);
    }
    void create_function_signatures_run() { //4 bytes: signatures count, 8 bytes: entity_id, 1 byte: number of arguments, 1 byte: argument's type, 8 bytes: entity_id. 0 - byte, 1 - two_bytes, 2 - four_bytes, 3 - eight_bytes, 4 - pointer
        std::uint64_t run_size = sizeof(std::uint32_t);
        auto saved_position = this->write_run_header(function_signatures_run);

        this->write_4_bytes(static_cast<std::uint32_t>(this->file_structure->functions.size()));
        for (const structure_builder::function& fnc : this->file_structure->functions) {
            this->write_8_bytes(fnc.id);
            std::size_t function_arguments_count = fnc.arguments.size();
            if (function_arguments_count > max_function_arguments_count) {
                this->add_new_logic_error(translator_error_type::too_many_function_arguments);
            }

            this->write_1_byte(static_cast<std::uint8_t>(function_arguments_count));
            for (const structure_builder::regular_variable& var: fnc.arguments) {
                this->write_1_byte(convert_type_to_uint8(var.type));
                this->write_8_bytes(var.id);

                run_size += 9;
            }

            run_size += 9;
        }

        this->write_run_footer(saved_position, run_size);
    }
    void create_function_body_run(const structure_builder::function& func) { //8 bytes: signature's entity_id, 4 bytes: number of local variables, 1 byte: local variable_type, 8 bytes: local variable entity_id
        std::uint64_t run_size = 12; //8 bytes: signature's entity_id, 4 bytes: number of local variables
        auto saved_position = this->write_run_header(function_body_run);

        this->write_8_bytes(func.id);
        this->write_4_bytes(static_cast<std::uint32_t>(func.locals.size()));

        for (const structure_builder::regular_variable& var : func.locals) {
            this->write_1_byte(convert_type_to_uint8(var.type));
            this->write_8_bytes(var.id);

            run_size += 9;
        }

        std::map operation_codes{ this->get_operation_codes() };
        std::vector filters_list{ this->get_instruction_filters() };
        for (const structure_builder::instruction& current_instruction : func.body) {
            this->check_logic_errors(filters_list, current_instruction);

            std::vector<unsigned char> instruction_symbols = instruction_encoder::encode_instruction(current_instruction, operation_codes);
            run_size += instruction_symbols.size();

            this->write_n_bytes(reinterpret_cast<char*>(instruction_symbols.data()),
                                instruction_symbols.size());
        }
        
        for (instruction_check* check : filters_list) {
            delete check;
        }

        this->write_run_footer(saved_position, run_size);
    }
    void create_exposed_functions_run() { //8 bytes: preferred stack size, 8 bytes: starting function, 8 bytes: exposed functions count, 8 bytes: exposed function's entity_id, 1 byte: exposed function name size, exposed function name
        std::uint64_t run_size = 24;
        auto saved_position = this->write_run_header(program_exposed_functions_run);

        auto found_main = std::ranges::find(
            this->file_structure->exposed_functions,
            this->file_structure->main_function
        );

        if (found_main == this->file_structure->exposed_functions.end()) {
            this->logic_errors.push_back(translator_error_type::main_not_exposed);
        }

        this->write_8_bytes(this->file_structure->stack_size);
        this->write_8_bytes(this->file_structure->main_function->id);
        this->write_8_bytes(this->file_structure->exposed_functions.size());
        for (structure_builder::function* exposed_function : this->file_structure->exposed_functions) {
            this->write_8_bytes(exposed_function->id);
            this->write_1_byte(static_cast<std::uint8_t>(exposed_function->name.size()));
            this->write_n_bytes(exposed_function->name.c_str(), exposed_function->name.size());

            run_size += 9 + exposed_function->name.size();
        }

        this->write_run_footer(saved_position, run_size);
    }
    void create_program_strings_run() { //8 bytes - amount of strings, 8 bytes - string id, 8 bytes - string size, string itself
        std::uint64_t run_size = 8;
        auto saved_position = this->write_run_header(program_strings_run);

        this->write_8_bytes(this->file_structure->program_strings.size());
        for (const auto& key_string : this->file_structure->program_strings) {
            std::size_t string_size = key_string.second.value.size();

            this->write_8_bytes(key_string.second.id);
            this->write_8_bytes(string_size);

            this->write_n_bytes(key_string.second.value.c_str(), string_size);
            run_size += 16 + string_size;
        }

        this->write_run_footer(saved_position, run_size);
    }

    template<typename T>
    std::uint64_t generic_write_element(const std::list<T>& list, const std::string& separator, const std::string& prefix_name) {
        std::uint64_t accumulator = 0;
        for (const T& element : list) {
            std::uint16_t element_name_size =
                static_cast<std::uint16_t>(element.name.size() + prefix_name.size() + separator.size());

            std::string combined_name{ prefix_name + separator + element.name };

            this->write_8_bytes(element.id);
            this->write_2_bytes(element_name_size);
            this->write_n_bytes(combined_name.c_str(), element_name_size);

            accumulator += 10 + element_name_size;
        }

        return accumulator;
    }

    void create_debug_run() {
        std::uint64_t run_size = 0;
        auto saved_position = this->write_run_header(program_debug_run);

        for (const auto& current_string : this->file_structure->program_strings) {
            std::uint16_t program_string_name_size =
                static_cast<std::uint16_t>(current_string.first.size());

            run_size += 10 + program_string_name_size;

            this->write_8_bytes(current_string.second.id);
            this->write_2_bytes(program_string_name_size);
            this->write_n_bytes(current_string.first.c_str(), program_string_name_size);
        }

        run_size += this->generic_write_element<structure_builder::engine_module>(
            this->file_structure->modules,
            "",
            ""
        );

        for (const structure_builder::engine_module& current_module : this->file_structure->modules) {
            run_size += this->generic_write_element<structure_builder::module_function>(
                current_module.functions_names,
                "(module function)",
                current_module.name
            );
        }

        run_size += this->generic_write_element<structure_builder::function>(
            this->file_structure->functions,
            "",
            ""
        );

        for (const structure_builder::function& current_function : this->file_structure->functions) {
            run_size += this->generic_write_element<structure_builder::regular_variable>(
                current_function.arguments,
                "(argument)",
                current_function.name
            );

            run_size += this->generic_write_element<structure_builder::regular_variable>(
                current_function.locals,
                "(local)",
                current_function.name
            );

            run_size += this->generic_write_element<structure_builder::jump_point>(
                current_function.jump_points,
                "(jump point)",
                current_function.name
            );
        }

        this->write_run_footer(saved_position, run_size);
    }

public:
    bytecode_translator(structure_builder::file* file, std::ostream* file_stream)
        :file_structure{file},
        out_stream{file_stream}
    {}

    void reset_file_structure(structure_builder::file* file) {
        this->file_structure = file;
    }
    void reset_out_stream(std::ostream* stream) {
        this->out_stream = stream;
    }

    void start(bool include_debug) {
        this->create_modules_run();
        this->create_jump_points_run();
        this->create_function_signatures_run();
        this->create_exposed_functions_run();
        this->create_program_strings_run();

        if (include_debug) {
            this->create_debug_run();
        }

        for (structure_builder::function& function : this->file_structure->functions) {
            this->create_function_body_run(function);
        }
    }

    const std::vector<translator_error_type>& errors() const { return this->logic_errors; }
};

#endif
