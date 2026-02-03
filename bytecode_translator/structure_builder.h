#ifndef STRUCTURE_BUILDER
#define STRUCTURE_BUILDER

#include <algorithm>
#include <vector>
#include <tuple>
#include <string>
#include <cstdlib>
#include <list>
#include <stdexcept>
#include <format>
#include <cerrno>
#include <filesystem>
#include <memory>
#include <optional>

#include "source_file_token.h"

#include "../generic_parser/token_generator.h"
#include "../generic_parser/read_map.h"

class structure_builder {
public:
    using entity_id = std::uint64_t;
    using line_type = std::uint64_t;
    using immediate_type = std::uint64_t;

    enum class parameters_enumeration {
        if_defined_if_not_defined_pop_check,
        inside_comment,
        names_stack,
        active_parsing_files
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
    struct immediate_variable;
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
        virtual void visit(source_file_token, const immediate_variable*, bool) = 0;
        virtual void visit(source_file_token, const function_address*, bool) = 0;
        virtual void visit(source_file_token, const module_variable*, bool) = 0;
        virtual void visit(source_file_token, const module_function_variable*, bool) = 0;
        virtual void visit(source_file_token, const jump_point_variable*, bool) = 0;
        virtual void visit(source_file_token, const string_constant*, bool) = 0;
        virtual ~variable_visitor() noexcept = default;
    };

    struct variable {
        virtual void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) = 0;
        virtual ~variable() = default;
    };

    struct immediate_variable : variable {
        immediate_type imm_val;
        source_file_token type;

        immediate_variable(source_file_token variable_type)
            :variable{},
            imm_val{ 0 },
            type{ variable_type }
        {}

        immediate_variable(source_file_token variable_type, immediate_type value)
            :variable{},
            imm_val{ value },
            type{ variable_type }
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct regular_variable : variable, entity {
        source_file_token type;
        std::string name;

        regular_variable(entity_id object_id, source_file_token variable_type) 
            :variable{},
            entity{ object_id },
            type{ variable_type }
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct function_address : variable {
        function* func;

        function_address()
            :variable{},
            func{nullptr}
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct pointer_dereference : variable {
        regular_variable* pointer_variable;
        std::vector<regular_variable*> derefernce_indexes;

        pointer_dereference()
            :variable{},
            pointer_variable{nullptr}
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct jump_point_variable : variable {
        jump_point* point;

        jump_point_variable(jump_point* jump_point)
            :variable{},
            point{ jump_point }
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct module_variable : variable {
        engine_module* mod;

        module_variable(engine_module* module_pointer)
            :variable{},
            mod{ module_pointer }
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct module_function_variable : variable {
        module_function* func;

        module_function_variable(module_function* mod_func)
            :variable{},
            func{mod_func}
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct string_constant : variable {
        string* value;
        string_constant(string* string_value)
            :value{ string_value }
        {}

        void visit(source_file_token active_type, variable_visitor* visitor, bool is_signed) override {
            visitor->visit(active_type, this, is_signed);
        }
    };

    struct instruction {
        source_file_token instruction_type; //Only tokens that correspond to instructions are allowed
        
        std::deque<pointer_dereference> dereferences; //functions can combine many memory dereferences, regular variables, immediates in one instruction
        std::deque<immediate_variable> immediates;
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

    struct jump_point : entity {
        std::uint32_t index;
        std::string name;

        jump_point(entity_id object_id, std::uint32_t point_index, std::string&& point_name)
            :entity{ object_id },
            index{ point_index },
            name { std::move(point_name) }
        {}

        jump_point(entity_id object_id, std::uint32_t point_index, const std::string& point_name)
            :entity{ object_id },
            index{ point_index },
            name{ point_name }
        {}
    };

    struct function : entity {
        std::list<regular_variable> arguments;
        std::list<regular_variable> locals; //You can't use variables BEFORE their declaration, but in byte code used variables will be known at compile time. Also decl is not an actual instruction, that's why there is a "source_file_token::special_instruction" before it.

        std::list<jump_point> jump_points;
        std::list<instruction> body;

        std::string name;
        function(entity_id object_id, std::string&& function_name)
            :entity{ object_id },
            name{ std::move(function_name) }
        {}
    };

    struct module_function : entity {
        std::string name;
        module_function(entity_id object_id, std::string&& module_name)
            :entity{ object_id },
            name{ std::move(module_name) }
        {}
    };

    struct engine_module : entity {
        std::string name;
        std::list<module_function> functions_names;

        engine_module(entity_id object_id, std::string&& module_name)
            :entity{ object_id },
            name{ std::move(module_name) },
            functions_names{}
        {}
    };

    struct string : entity {
        std::string value;
        string(entity_id object_id)
            :entity{ object_id },
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
                this->get_last_instruction().operands_in_order.emplace_back(token, var, is_signed);
            }

            void add_new_instruction(source_file_token token) {
                this->get_current_function().body.emplace_back(token);
            }

            decltype(auto) get_last_operand() {
                return this->get_last_instruction().operands_in_order.back();
            }

            decltype(auto) find_argument_variable_by_name(const std::string& name) {
                return std::ranges::find_if(this->get_current_function().arguments,
                                            [&name](const regular_variable& var) {
                                                return var.name == name;
                                            });
            }

            decltype(auto) find_local_variable_by_name(const std::string& name) {
                return std::ranges::find_if(this->get_current_function().locals,
                                            [&name](const regular_variable& var) {
                                                return var.name == name;
                                            });
            }

            template<typename type>
            void map_operand_with_variable(
                const std::string& name, 
                type** out, 
                generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>& parser
            ) {
                auto found_argument = this->find_argument_variable_by_name(name);
                if (found_argument != this->get_current_function().arguments.end()) {
                    *out = &*found_argument;
                    return;
                }

                auto found_local = this->find_local_variable_by_name(name);
                if (found_local != this->get_current_function().locals.end()) {
                    *out = &*found_local;
                    return;
                }

                parser.exit_with_error("Name '" + name + "' does not exist.");
            }
        };

        struct names_remapping {
        private:
            std::vector<std::pair<std::string, std::string>> remappings; //used with redefine
            auto find_remapped_name(const std::string& name_to_find) const {
                auto found_defined_name = std::ranges::find_if(this->remappings,
                                                               [&name_to_find](const std::pair<std::string, std::string>& val) {
                                                                   return name_to_find == val.first;
                                                               });

                return found_defined_name;
            }

        public:
            auto merge(names_remapping& other) {
                // This is actually diabolical.
                for (auto& [other_key, other_value] : other.remappings) {
                    auto found = this->find_remapped_name(other_key);
                    if (found == this->remappings.end()) {
                        this->remappings.emplace_back(std::move(other_key), std::move(other_value));
                    }
                    else if (!found->second.empty() || !other_value.empty()) {
                        // Allow repeated redefinitions only if they are empty.
                        return std::optional{ std::make_tuple(
                            std::move(other_key), std::move(other_value),
                            found->first, found->second
                        ) };
                    }
                }

                return std::optional<std::tuple<std::string, std::string, std::string, std::string>>{};
            }

            void add(std::string what, std::string new_value) {
                this->remappings.emplace_back(std::move(what), std::move(new_value));
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
                    int base;
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
                    if (static_cast<std::size_t>(last_symbol - value.c_str()) == value.size()) {
                        if (errno == ERANGE || result != static_cast<T>(result)) {
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
                if (found_name != this->remappings.end() && !found_name->second.empty()) {
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

        void add_function_address_argument(
            file& file_structure, 
            builder_parameters& helper_parameters, 
            generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>& read_map
        ) {
            helper_parameters.active_function.get_last_instruction().func_addresses.emplace_back(); //add new function address to the list function addresses of specific instruction

            function_address* function_address = &helper_parameters.active_function.get_last_instruction().func_addresses.back();
            std::string function_name = helper_parameters.name_translations.translate_name(read_map.get_token_generator_name());
            auto found_function = std::ranges::find_if(file_structure.functions, //try to find function with specific name inside functions list
                                                       [&function_name](const function& func) {
                                                           return func.name == function_name;
                                                       });

            if (found_function == file_structure.functions.end()) { //if search was unsuccessful then exit with error
                read_map.exit_with_error("Function with name '" + function_name + "' does not exist.");
                return;
            }

            function* function = &*found_function;
            function_address->func = function; //bind function address argument with specific function

            helper_parameters.active_function.add_new_operand_to_last_instruction(source_file_token::function_address_argument_keyword, function_address, false);
        }
    };

    using read_map_type = generic_parser::read_map<source_file_token, context_key, file, builder_parameters, parameters_enumeration>;

private:
    line_type error_line;
    generic_parser::token_generator<source_file_token, context_key>* generator;

    read_map_type parse_map;
    void configure_parse_map();

    builder_parameters helper{};
    file output_file_structure;

public:
    structure_builder(
        std::vector<std::pair<std::string, source_file_token>>* names_stack, 
        generic_parser::token_generator<source_file_token, context_key>* token_generator,
        std::shared_ptr<std::vector<std::filesystem::path>> active_parsing_files
    )
        :error_line{ 1 },
        generator{ token_generator },
        parse_map{ source_file_token::end_of_file, source_file_token::name, token_generator }
    {
        assert(active_parsing_files != nullptr && 
            !active_parsing_files->empty() && 
            "At least one active file for parsing must be present.");

        this->parse_map.get_parameters_container()
            .assign_parameter(parameters_enumeration::if_defined_if_not_defined_pop_check, false)
            .assign_parameter(parameters_enumeration::inside_comment, std::pair<bool, std::string>{ false, "" })
            .assign_parameter(parameters_enumeration::names_stack, names_stack)
            .assign_parameter(parameters_enumeration::active_parsing_files, active_parsing_files);

        this->configure_parse_map();
    }

    std::pair<line_type, std::string> error() const { return { this->error_line, this->parse_map.error() }; }
    bool is_working() { return this->parse_map.is_working(); }
    void handle_token(source_file_token token) {
        auto& [just_left_comment, saved_name] = this->parse_map
            .get_parameters_container()
            .retrieve_parameter<std::pair<bool, std::string>>(parameters_enumeration::inside_comment);

#ifdef __clang__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch-default"

#endif

        switch (token) {  // NOLINT(clang-diagnostic-switch)
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

#ifdef __clang__

#pragma clang diagnostic pop

#endif

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
