#ifndef MODULE_FILE_COMPONENTS_H
#define MODULE_FILE_COMPONENTS_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <bit>
#include <string>
#include <iostream>

#include "module_part.h"

namespace module_mediator::parser::components {
    using module_callable_function_type = return_value(*)(arguments_string_type);
    class function {
        std::string name; //this function's name. used to identify its index
        arguments_string_type arguments_symbols; //a sequence of bytes that represent this function's arguments

        bool visible;
        module_callable_function_type address;

        void delete_arguments_symbols() {
            delete this->arguments_symbols;
            this->arguments_symbols = nullptr;
        }

        void move_value(function& old_value) {
            this->arguments_symbols = old_value.arguments_symbols;
            this->name = std::move(old_value.name);
            this->visible = old_value.visible;
            this->address = old_value.address;

            old_value.arguments_symbols = nullptr;
        }

        bool compare_arguments_strings_arguments_count(arguments_string_type other_arguments_symbols) const {
            return other_arguments_symbols[0] == this->arguments_symbols[0];
        }

        bool check_arguments_strings_arguments_types(arguments_string_type other_arguments_symbols) const {
            return std::memcmp(other_arguments_symbols, this->arguments_symbols, static_cast<std::size_t>(this->arguments_symbols[0]) + 1) == 0;
        }

    public:
        function()
            :arguments_symbols{ nullptr },
            visible{ false },
            address{ nullptr }
        {
        }

        function(
            std::string&& function_name, 
            module_callable_function_type function_address, 
            arguments_string_type function_arguments_symbols, 
            bool visible_function)
            :name{ std::move(function_name) },
            arguments_symbols{ function_arguments_symbols },
            visible{ visible_function },
            address{ function_address }
        {
        }

        function(const function&) = delete; //this type is used with std::vector which does not generally require copy constructor (only with special functions)
        void operator= (const function&) = delete;

        function(function&& old_value) noexcept {
            this->move_value(old_value);
        }

        function& operator= (function&& old_value) noexcept {
            this->delete_arguments_symbols();
            this->move_value(old_value);

            return *this;
        }

        bool compare_names(std::string_view other_name) const { return this->name == other_name; }
        bool compare_arguments_types(arguments_string_type other_arguments_symbols) const { //true if equal
            assert(other_arguments_symbols && "null pointer");
            if (this->arguments_symbols == nullptr) { //if function has no arguments symbols it means that it automatically accepts all arguments
                return true;
            }

            if (this->compare_arguments_strings_arguments_count(other_arguments_symbols)) {
                return this->check_arguments_strings_arguments_types(other_arguments_symbols);
            }

            return false;
        }

        bool is_visible() const { return this->visible; }
        return_value call(arguments_string_type arguments) const {
            return this->address(arguments);
        }

        ~function() {
            this->delete_arguments_symbols();
        }
    };

    class engine_module {
        std::string name; //used to find engine_module's index
        HMODULE loaded_module;

        std::vector<function> functions;

        void move_value(engine_module& old_value) {
            this->name = std::move(old_value.name);
            this->functions = std::move(old_value.functions);

            this->loaded_module = old_value.loaded_module;
            old_value.loaded_module = nullptr;
        }

        void free_resources() {
            this->free_module();
        }

        void load_module(const std::string& module_path) {
            this->loaded_module = LoadLibraryA(module_path.c_str());
            if (this->loaded_module == nullptr) {
                std::cerr << "Unable to load one of the modules. Process will be aborted."
                    << " (Path: " << module_path << ')' << '\n';

                std::terminate();
            }
        }

        void free_module() {
            if (this->loaded_module != nullptr) {
                FARPROC free = GetProcAddress(this->loaded_module, "free_m");
                if (free != nullptr) {
                    std::bit_cast<void(*)()>(free)();  // NOLINT(bugprone-bitwise-pointer-cast)
                }

                BOOL freed_library = FreeLibrary(this->loaded_module);
                if (!freed_library) {
                    std::cerr << "Unable to correctly dispose one of the modules. Process will be aborted."
                        << " (Name: " << this->name << ')' << '\n';

                    std::terminate();
                }
            }
        }

        void initialize_module(module_part* mediator) {
            FARPROC initialize = GetProcAddress(this->loaded_module, "initialize_m");
            if (initialize == nullptr) {
                std::cerr << "One of the modules does not define the initialize_m function. Process will be aborted."
                    << "(Name: " << this->name << ')' << '\n';

                std::terminate();  
            }
            
            std::bit_cast<void(*)(module_part*)>(initialize)(mediator); // NOLINT(bugprone-bitwise-pointer-cast)
        }

    public:
        engine_module(
            std::string&& module_name,
            const std::string& module_path,
            module_part* mediator //pointer to module_part allows to access some of the engine_module_mediator functions
        )
            :name{ std::move(module_name) },
            loaded_module{ nullptr }
        {
            this->load_module(module_path);
            this->initialize_module(mediator);
        }

        engine_module(const engine_module&) = delete;
        void operator= (const engine_module&) = delete;

        engine_module(engine_module&& old_value) noexcept {
            this->move_value(old_value);
        }

        engine_module& operator= (engine_module&& old_value) noexcept {
            this->free_resources();
            this->move_value(old_value);

            return *this;
        }

        const std::string& get_name() const {
            return this->name;
        }

        bool compare_names(std::string_view other_name) const { return this->name == other_name; }
        std::size_t find_function_index(std::string_view other_name) const {
            for (std::size_t find_index = 0, size = this->functions.size(); find_index < size; ++find_index) {
                if (this->functions[find_index].compare_names(other_name)) {
                    return find_index;
                }
            }

            return module_part::function_not_found;
        }

        const function& get_function(std::size_t index) const {
            return this->functions.at(index);
        }

        bool add_function(const std::string& other_name, std::string&& export_name, arguments_string_type arguments_string, bool is_visible) {
            FARPROC loaded_function = GetProcAddress(this->loaded_module, other_name.c_str());
            if (loaded_function == nullptr)
                return false;

            this->functions.emplace_back(
                std::move(export_name),
                    std::bit_cast<module_callable_function_type>(loaded_function),  // NOLINT(bugprone-bitwise-pointer-cast)
                    arguments_string,
                    is_visible

            );

            return true;
        }

        ~engine_module() {
            this->free_resources();
        }
    };
}

#endif
