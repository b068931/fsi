#ifndef ENGINE_MODULE_MEDIATOR_H
#define ENGINE_MODULE_MEDIATOR_H

#define NOMINMAX 
#include <Windows.h>
#include <cassert>
#include <string_view>
#include <stdexcept>
#include <filesystem>

#include "file_builder.h"
#include "parser_options.h"
#include "file_components.h"

#include "../logger_module/logging.h"
#include "../generic_parser/parser_facade.h"

/*
* "When an evaluation of an expression writes to a memory location and another evaluation reads or modifies
* the same memory location, the expressions are said to conflict."
* find_module_index, find_function_index, call_module should not modify memory locations in any way
*/

namespace module_mediator::exceptions {
    class function_not_visible : public std::logic_error {
    public:
        function_not_visible(const std::string& string)
            :std::logic_error{ string }
        {}
    };
    class invalid_arguments_string : public std::logic_error {
    public:
        invalid_arguments_string(const std::string& string)
            :std::logic_error{ string }
        {}
    };
}

namespace module_mediator {
    /// <summary>
    /// Mediates access to engine modules, managing their lifecycle and providing a safe interface for module function calls. 
    /// This class implements the mediator pattern to coordinate interactions between different engine modules loaded from 
    /// configuration files. Only a single instance may ever exist at any moment.
    /// </summary>
    class engine_module_mediator {
        class module_part_implementation {
            static inline std::atomic<engine_module_mediator*> mediator{ nullptr };

        public:
            static void initialize(engine_module_mediator* mediator_instance) {
                assert(mediator.load(std::memory_order_relaxed) == nullptr && "Instance already initialized.");
                mediator.store(mediator_instance, std::memory_order_relaxed);
            }

            static void reset() {
                assert(mediator.load(std::memory_order_relaxed) != nullptr && "Instance never initialized.");
                mediator.store(nullptr, std::memory_order_relaxed);
            }

            static std::size_t find_function_index(std::size_t module_index, const char* name) {
                try {
                    return mediator.load(std::memory_order_relaxed)->find_function_index(
                        module_index, name);
                }
                catch (const std::out_of_range&) {
                    return module_part::function_not_found;
                }
            }

            static std::size_t find_module_index(const char* name) {
                return mediator.load(std::memory_order_relaxed)->find_module_index(name);
            }

            static return_value call_module(
                std::size_t module_index, 
                std::size_t function_index, 
                arguments_string_type arguments_string
            ) {
                try {
                    return mediator.load(std::memory_order_relaxed)->call_module(
                        module_index, function_index, arguments_string);
                }
                catch ([[maybe_unused]] const std::exception& exc) {
                    std::cerr << std::format(
                        "*** CALL_MODULE FAILED. ERROR:\n{}", 
                        exc.what()) << '\n';

                    std::terminate();
                }
            }

            static return_value call_module_visible_only(
                std::size_t module_index, 
                std::size_t function_index, 
                arguments_string_type arguments_string, 
                void(*error_callback)(module_part::call_error)
            ) {
                // ReSharper disable once CppInitializedValueIsAlwaysRewritten
                module_part::call_error error{ module_part::call_error::no_error };

                try {
                    return mediator.load(std::memory_order_relaxed)->call_module_visible_only(
                        module_index, function_index, arguments_string);
                }
                catch (const exceptions::function_not_visible&) {
                    error = module_part::call_error::function_is_not_visible;
                }
                catch (const std::out_of_range&) {
                    error = module_part::call_error::unknown_index;
                }
                catch (const exceptions::invalid_arguments_string&) {
                    error = module_part::call_error::invalid_arguments_string;
                }

                error_callback(error);
                return 0;
            }
        };

        std::vector<parser::components::engine_module> loaded_modules;
        module_part* part_implementation{};

        const parser::components::engine_module& get_module(std::size_t module_index) const {
            return this->loaded_modules.at(module_index);
        }

        std::size_t find_module_index(std::string_view name) const {
            for (std::size_t index = 0, size = this->loaded_modules.size(); index < size; ++index) {
                if (this->loaded_modules[index].compare_names(name)) {
                    return index;
                }
            }

            return module_part::module_not_found;
        }

        std::size_t find_function_index(std::size_t module_index, std::string_view name) const {
            return this->get_module(module_index).find_function_index(name);
        }

        return_value call_module(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string) {
            if (this->get_module(module_index).get_function(function_index).compare_arguments_types(arguments_string)) { //true if arguments match
                return this->get_module(module_index).get_function(function_index).call(arguments_string);
            }

            throw exceptions::invalid_arguments_string{ "Invalid arguments string used." };
        }

        return_value call_module_visible_only(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string) {
            if (this->get_module(module_index).get_function(function_index).is_visible()) {
                return this->call_module(module_index, function_index, arguments_string);
            }

            throw exceptions::function_not_visible{ "This function is not visible." };
        }

    public:
        engine_module_mediator()
        {
            module_part_implementation::initialize(this);
            this->part_implementation = new module_part{
                .find_function_index = &module_part_implementation::find_function_index,
                .find_module_index = &module_part_implementation::find_module_index,
                .call_module = &module_part_implementation::call_module,
                .call_module_visible_only = &module_part_implementation::call_module_visible_only
            };
        }

        engine_module_mediator(const engine_module_mediator&) = delete;
        engine_module_mediator& operator= (const engine_module_mediator&) = delete;

        engine_module_mediator(engine_module_mediator&&) = delete;
        engine_module_mediator& operator= (engine_module_mediator&&) = delete;

        std::string load_modules(const std::filesystem::path& file_name) {
            generic_parser::parser_facade<
                parser::components::engine_module_builder::file_tokens, 
                parser::components::engine_module_builder::context_keys, 
                parser::components::engine_module_builder
            > parser{
                parser::parser_options::keywords,
                parser::parser_options::contexts,
                parser::components::engine_module_builder::file_tokens::name,
                parser::components::engine_module_builder::file_tokens::end_of_file,
                parser::components::engine_module_builder::context_keys::main_context,
                this
            };

            try {
                parser.start(file_name);
            }
            catch (const std::exception& exc) {
                return exc.what();
            }

            this->loaded_modules = parser.get_builder_value();
            return parser.error();
        }

        module_part* get_module_part() {
            assert(this->part_implementation != nullptr && "Module part implementation is null.");
            return this->part_implementation;
        }

        ~engine_module_mediator() {
            assert(this->part_implementation != nullptr && "Module part implementation is null.");

            delete this->part_implementation;
            module_part_implementation::reset();
        }
    };
}

#endif
