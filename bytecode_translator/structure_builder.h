#ifndef STRUCTURE_BUILDER
#define STRUCTURE_BUILDER

#include <vector>
#include <tuple>
#include <iostream>
#include <string>
#include <cstdlib>
#include <list>
#include <stdexcept>
#include <format>
#include <cerrno>

#include "../generic_parser/token_generator.h"
#include "../generic_parser/read_map.h"

/*
* generally speaking, this entire file is FUCKED UP.
* tons of code depend on pointers, most of the time this means nothing (this is c++ after all).
* but here it makes heavy use of pointers to elements in STL structures.
* and these structures tend to invalidate these pointers, invoking UB (before i used vectors,
* and it didn't end well, as you might have guessed).
* so std::list, std::deque, etc. are the only options.
*/

//this class is used to convert FSI file contents to objects. (FSI file is a source file, FSI are random letters and mean nothing)
class structure_builder {
public:
    using entity_id = std::uint64_t;
    using line_type = std::uint64_t;
    using immediate_type = std::uint64_t;

    enum class error_type {
        no_error,
        outside_function,
        inside_module_import,
        IMPORT_was_expected,
        import_START_was_expected,
        coma_or_import_end_were_expected,
        special_symbol,
        DEFINE_name_expected,
        REDEFINE_name_expected,
        IFDEF_name_expected,
        IFNDEF_name_expected,
        STACK_SIZE_name_expected,
        invalid_number,
        unexpected_type,
        can_not_declare_variable,
        DECL_name,
        FUNCTION_name,
        args_start_expected,
        args_end_or_coma,
        FUNCTION_args_unexpected_type,
        FUNCTION_args_unexpected_token,
        FUNCTION_args_empty_name,
        FUNCTION_body_start_was_expected,
        FUNCTION_body_token,
        INSTRUCTION_token,
        name_does_not_exist
    };
    enum class source_file_token {
        end_of_file,
        name,
        coma, //,
        comment_start, //tokens inside comments will be simply ignored
        comment_end,
        import_start,  //from ... import <> (<)
        import_end,	   //(>)
        from_keyword,
        import_keyword,
        special_instruction, //.redefine and other
        redefine_keyword,
        define_keyword,
        undefine_keyword,
        if_defined_keyword,
        if_not_defined_keyword,
        endif_keyword,
        stack_size_keyword,
        declare_keyword,
        main_function_keyword,
        include_keyword,
        function_declaration_keyword, //function
        function_args_start, //(
        function_args_end, //)
        sizeof_argument_keyword,
        one_byte_type_keyword,
        two_bytes_type_keyword,
        four_bytes_type_keyword,
        eight_bytes_type_keyword,
        pointer_type_keyword,
        function_body_start, //{
        expression_end, //;
        move_instruction_keyword,
        add_instruction_keyword,
        signed_add_instruction_keyword,
        subtract_instruction_keyword,
        signed_subtract_instruction_keyword,
        multiply_instruction_keyword,
        signed_multiply_instruction_keyword,
        divide_instruction_keyword,
        signed_divide_instruction_keyword,
        compare_instruction_keyword,
        increment_instruction_keyword,
        decrement_instruction_keyword,
        jump_instruction_keyword,
        jump_equal_instruction_keyword,
        jump_not_equal_instruction_keyword,
        jump_greater_instruction_keyword,
        jump_greater_equal_instruction_keyword,
        jump_less_instruction_keyword,
        jump_less_equal_instruction_keyword,
        jump_above_instruction_keyword,
        jump_above_equal_instruction_keyword,
        jump_below_instruction_keyword,
        jump_below_equal_instruction_keyword,
        dereference_start, //[
        dereference_end, //]
        module_call, //->
        module_return_value, //:
        jump_point, //@
        function_body_end, //}
        immediate_argument_keyword,
        new_line,
        function_address_argument_keyword,
        signed_argument_keyword,
        variable_argument_keyword,
        pointer_dereference_argument_keyword,
        no_return_module_call_keyword,
        function_call,
        jump_point_argument_keyword,
        bit_and_instruction_keyword,
        bit_or_instruction_keyword,
        bit_xor_instruction_keyword,
        bit_not_instruction_keyword,
        save_value_instruction_keyword,
        load_value_instruction_keyword,
        move_pointer_instruction_keyword,
        bit_shift_left_instruction_keyword,
        bit_shift_right_instruction_keyword,
        get_function_address_instruction_keyword,
        define_string_keyword,
        string_separator,
        string_argument_keyword,
        copy_string_instruction_keyword
    };
    enum class parameters_enumeration {
        ifdef_ifndef_pop_check,
        inside_comment,
        names_stack
    };
    enum class context_key {
        main_context,
        inside_string,
        inside_include,
        inside_comment
    };

    struct function;
    struct jump_point;
    struct string;
    struct engine_module;
    struct module_function;

    struct entity { //every jump point, variable, function has its own id (later this id will be used in byte code)
        entity_id id;
    };

    struct pointer_dereference;
    struct imm_variable;
    struct function_address;
    struct module_variable;
    struct module_function_variable;
    struct jump_point_variable;
    struct string_constant;
    struct regular_variable;

    class variable_visitor {
    public:
        virtual void visit(source_file_token, const pointer_dereference*, bool) = 0;
        virtual void visit(source_file_token, const regular_variable*, bool) = 0;
        virtual void visit(source_file_token, const imm_variable*, bool) = 0;
        virtual void visit(source_file_token, const function_address*, bool) = 0;
        virtual void visit(source_file_token, const module_variable*, bool) = 0;
        virtual void visit(source_file_token, const module_function_variable*, bool) = 0;
        virtual void visit(source_file_token, const jump_point_variable*, bool) = 0;
        virtual void visit(source_file_token, const string_constant*, bool) = 0;
    };

    struct variable {
        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) = 0;
        virtual ~variable() = default;
    };
    struct imm_variable : public variable {
        immediate_type imm_val;
        source_file_token type;

        imm_variable(source_file_token type)
            :variable{},
            imm_val{0},
            type{type}
        {}

        imm_variable(source_file_token type, immediate_type value)
            :variable{},
            imm_val{value},
            type{type}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct regular_variable : public variable, public entity {
        source_file_token type;
        std::string name;

        regular_variable(entity_id id, source_file_token type) 
            :variable{},
            entity{id},
            type{type}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct function_address : public variable {
        function* func;

        function_address()
            :variable{},
            func{nullptr}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct pointer_dereference : public variable {
        regular_variable* pointer_variable;
        std::vector<regular_variable*> derefernce_indexes;

        pointer_dereference()
            :variable{},
            pointer_variable{nullptr}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct jump_point_variable : public variable {
        jump_point* point;

        jump_point_variable(jump_point* point)
            :variable{},
            point{point}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct module_variable : public variable {
        engine_module* mod;

        module_variable(engine_module* mod)
            :variable{},
            mod{mod}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct module_function_variable : public variable {
        module_function* func;

        module_function_variable(module_function* mod_func)
            :variable{},
            func{mod_func}
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };
    struct string_constant : public variable {
        string* value;
        string_constant(string* value)
            :value{ value }
        {}

        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct instruction {
        source_file_token instruction_type; //Only tokens that correspond to instructions are allowed
        
        std::deque<pointer_dereference> dereferences; //functions can combine many pointer dereferences, regular variables, immediates in one instruction
        std::deque<imm_variable> immediates;
        std::deque<function_address> func_addresses;
        std::deque<module_variable> modules;
        std::deque<module_function_variable> module_functions;
        std::deque<jump_point_variable> jump_variables;
        std::deque<string_constant> strings;

        std::vector<std::tuple<source_file_token, variable*, bool>> operands_in_order; //source_file_token is an actual type used in instruction

        instruction(source_file_token instruction)
            :instruction_type{instruction}
        {}
    };
    struct jump_point : public entity {
        std::uint32_t index;
        std::string name;

        jump_point(entity_id id, std::uint32_t index, std::string&& name)
            :entity{ id },
            name {std::move(name)},
            index{index}
        {}

        jump_point(entity_id id, std::uint32_t index, const std::string& name)
            :entity{ id },
            name{ name },
            index{ index }
        {}
    };
    struct function : public entity {
        std::list<regular_variable> arguments;
        std::list<regular_variable> locals; //You can't use variables BEFORE their declaration, but in byte code used variables will be known at compile time. Also decl is not an actual instruction, that's why there is a "source_file_token::special_instruction" before it.

        std::list<jump_point> jump_points;
        std::list<instruction> body;

        std::string name;
        function(entity_id id, std::string&& name)
            :entity{id},
            name{std::move(name)}
        {}
    };
    struct module_function : entity {
        std::string name;
        module_function(entity_id id, std::string&& name)
            :entity{id},
            name{std::move(name) }
        {}
    };
    struct engine_module : entity {
        std::string name;
        std::list<module_function> functions_names;

        engine_module(entity_id id, std::string&& module_name)
            :entity{id},
            name{std::move(module_name)},
            functions_names{}
        {}
    };
    struct string : public entity {
        std::string value;
        string(entity_id id)
            :entity{ id },
            value{}
        {}
    };
    struct file {
        std::uint64_t stack_size{ 0 };
        function* main_function{};
        
        std::vector<function*> exposed_functions;
        std::map<std::string, string> program_strings;

        std::list<engine_module> modules;
        std::list<function> functions;
    };

    class builder_parameters {
    public:
        struct current_function {
        private:
            function* current_function_value{};

        public:
            bool is_current_function_present() { return this->current_function_value != nullptr; }

            void set_current_function(function* value) {
                this->current_function_value = value;
            }
            function& get_current_function() {
                assert(current_function_value != nullptr && "oops");
                return *this->current_function_value;
            }

            decltype(auto) get_last_instruction() {
                return this->get_current_function().body.back();
            }
            void add_new_operand_to_last_instruction(source_file_token token, variable* var, bool is_signed) {
                this->get_last_instruction().operands_in_order.push_back({ token, var, is_signed });
            }
            void add_new_instruction(source_file_token token) {
                this->get_current_function().body.push_back({ token });
            }
            decltype(auto) get_last_operand() {
                return this->get_last_instruction().operands_in_order.back();
            }
            decltype(auto) find_argument_variable_by_name(const std::string& name) {
                return std::find_if(this->get_current_function().arguments.begin(), this->get_current_function().arguments.end(),
                    [&name](const regular_variable& var) {
                        return var.name == name;
                    });
            }
            decltype(auto) find_local_variable_by_name(const std::string& name) {
                return std::find_if(this->get_current_function().locals.begin(), this->get_current_function().locals.end(),
                    [&name](const regular_variable& var) {
                        return var.name == name;
                    });
            }

            template<typename type>
            void map_operand_with_variable(const std::string& name, type** out, generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>& parse_map) {
                auto found_argument = this->find_argument_variable_by_name(name);
                if (found_argument != this->get_current_function().arguments.end()) {
                    *out = &(*found_argument);
                    return;
                }

                auto found_local = this->find_local_variable_by_name(name);
                if (found_local != this->get_current_function().locals.end()) {
                    *out = &(*found_local);
                    return;
                }

                parse_map.exit_with_error("Name '" + name + "' does not exist.");
            }
        };
        struct names_remapping {
        private:
            std::vector<std::pair<std::string, std::string>> remappings; //used with redefine
            auto find_remapped_name(const std::string& name_to_find) const {
                auto found_defined_name = std::find_if(this->remappings.begin(), this->remappings.end(),
                    [&name_to_find](const std::pair<std::string, std::string>& val) {
                        return name_to_find == val.first;
                    });

                return found_defined_name;
            }

        public:
            void merge(names_remapping&& other) {
                //this is actually diabolical
                for (auto begin = other.remappings.begin(); begin != other.remappings.end(); ++begin) {
                    auto found = this->find_remapped_name(begin->first);
                    if (found == remappings.end()) {
                        remappings.push_back(std::move(*begin));
                    }
                }
            }

            void add(std::string what, std::string new_value) {
                this->remappings.push_back({ std::move(what), std::move(new_value) });
            }
            void remove(const std::string& what) {
                auto found_remapping = this->find_remapped_name(what);
                if (found_remapping != this->remappings.end()) {
                    this->remappings.erase(found_remapping);
                }
            }

            std::pair<std::string, std::string>& back() {
                return this->remappings.back();
            }
            bool has_remapping(const std::string& name) const {
                auto found_remapping = this->find_remapped_name(name);
                return found_remapping != this->remappings.end();
            }

            template<typename T>
            T translate_name_to_integer(std::string name) const {
                std::string value{ this->translate_name(std::move(name)) };
                if (!value.empty()) {
                    int base = 0;
                    switch (value[0]) {
                    case 'd':
                        base = 10;
                        break;
                    case 'b':
                        base = 2;
                        break;
                    case 'o':
                        base = 8;
                        break;
                    case 'h':
                        base = 16;
                        break;
                    default:
                        throw std::invalid_argument{ "Unknown base." };
                    }

                    char* last_symbol = nullptr;
                    unsigned long long result = std::strtoull(value.c_str() + 1, &last_symbol, base);
                    if ((last_symbol - value.c_str()) == value.size()) {
                        if ((errno == ERANGE) || (result != static_cast<T>(result))) {
                            throw std::invalid_argument{ "Value is too big to be converted." };
                        }
                        
                        return static_cast<T>(result);
                    }
                }
                
                throw std::invalid_argument{ std::format("Was unable to convert {} to a number.", value) };
            }
            std::string translate_name(std::string name) const { //translate redefined name to a normal name
                std::string generated_name = std::move(name);
                auto found_name = this->find_remapped_name(generated_name);
                if ((found_name != this->remappings.end()) && (found_name->second != "")) {
                    generated_name = found_name->second;
                }

                return generated_name;
            }
        };

        current_function active_function{};
        names_remapping name_translations{};

        line_type instruction_index; //used for jump points, resets back to zero when new function is declared

        std::list<engine_module>::iterator current_module{};
        std::map<std::string, string>::iterator current_string{};

        static entity_id get_id() {
            static entity_id id = 1;
            return id++;
        }
        void add_function_address_argument(file& output_file_structure, builder_parameters& helper, generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>& read_map) {
            helper.active_function.get_last_instruction().func_addresses.push_back(function_address{}); //add new function address to the list function addresses of specific instruction

            function_address* func = &helper.active_function.get_last_instruction().func_addresses.back();
            std::string function_name = helper.name_translations.translate_name(read_map.get_token_generator_name());
            auto found_function = std::find_if(output_file_structure.functions.begin(), output_file_structure.functions.end(), //try to find function with specific name inside functions list
                [&function_name](const function& func) {
                    return func.name == function_name;
                });
            if (found_function == output_file_structure.functions.end()) { //if search was unsuccessful then exit with error
                read_map.exit_with_error("Function with name '" + function_name + "' does not exist.");
                return;
            }

            function* function = &(*found_function);
            func->func = function; //bind function address argument with specific function

            auto found_exposed_function =
                std::find(output_file_structure.exposed_functions.begin(), output_file_structure.exposed_functions.end(), function);
            if (found_exposed_function == output_file_structure.exposed_functions.end()) { //insert exposed function if it hasn't been already added to the list
                output_file_structure.exposed_functions.push_back(function);
            }

            helper.active_function.add_new_operand_to_last_instruction(source_file_token::function_address_argument_keyword, func, false);
        }
    };
    using read_map_type = generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>;

private:
    line_type error_line;
    generic_parser::token_generator<structure_builder::source_file_token, context_key>* generator;

    read_map_type parse_map;
    void configure_parse_map();

    builder_parameters helper{};
    file output_file_structure;
public:
    structure_builder(
        std::vector<std::pair<std::string, structure_builder::source_file_token>>* names_stack, 
        generic_parser::token_generator<structure_builder::source_file_token, context_key>* token_generator
    )
        :generator{ token_generator },
        error_line{ 1 },
        parse_map{ source_file_token::end_of_file, source_file_token::name, token_generator }
    {
        this->parse_map
            .get_parameters_container()
            .assign_parameter(parameters_enumeration::ifdef_ifndef_pop_check, false)
            .assign_parameter(parameters_enumeration::inside_comment, std::pair<bool, std::string>{ false, "" })
            .assign_parameter(parameters_enumeration::names_stack, names_stack);

        this->configure_parse_map();
    }

    std::pair<line_type, std::string> error() const { return { this->error_line, this->parse_map.error() }; }
    bool is_working() { return this->parse_map.is_working(); }
    void handle_token(structure_builder::source_file_token token) {
        auto& [just_left_comment, saved_name] = this->parse_map
            .get_parameters_container()
            .retrieve_parameter<std::pair<bool, std::string>>(parameters_enumeration::inside_comment);

        switch (token) {
            case source_file_token::comment_start: {
                saved_name = this->generator->get_name();
                this->generator->set_current_context(context_key::inside_comment);

                return;
            }
            case source_file_token::comment_end: {
                this->generator->set_current_context(context_key::main_context);
                just_left_comment = true;

                return;
            }
            case source_file_token::new_line: {
                ++this->error_line;

                if (this->generator->get_additional_token() != source_file_token::end_of_file) { //check if the name is a keyword that has a special token
                    token = this->generator->get_additional_token();
                    this->generator->set_token_from_names_stack(true);
                }
                else {
                    token = source_file_token::name;
                }

                break;
            }
        }

        if (just_left_comment) {
            just_left_comment = false;
            if (!this->generator->is_name_empty() && !saved_name.empty()) {
                this->parse_map.exit_with_error("Incorrect syntax around comment.");
                return;
            }

            if (this->generator->is_name_empty()) {
                source_file_token result = this->generator->translate_string_through_names_stack(saved_name);
                if (result != source_file_token::end_of_file) {
                    if (token == source_file_token::name) {
                        this->generator->set_token_from_names_stack(true);
                        token = result;
                    }
                    else {
                        this->generator->set_additional_token(result);
                    }
                }

                this->generator->set_name(std::move(saved_name));
            }
        }

        if (!(this->generator->is_name_empty() && token == source_file_token::name)) { //empty names will not be handled in any way
            this->parse_map.handle_token(&this->output_file_structure, token, &this->helper);
        }
    }

    //line number is used when an error occurs
    file get_value() { return std::move(this->output_file_structure); }
    builder_parameters::names_remapping& get_names_translations() { return this->helper.name_translations; }
};

#endif