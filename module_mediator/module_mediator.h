#ifndef ENGINE_MODULE_MEDIATOR_H
#define ENGINE_MODULE_MEDIATOR_H

#define NOMINMAX 
#include <Windows.h>
#include <cassert>
#include <tuple>
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
	class engine_module_mediator {
        class module_part_implementation : public module_part {
            engine_module_mediator* mediator;

		public:
			module_part_implementation(engine_module_mediator* module_mediator)
				:mediator{ module_mediator }
			{
			}

			virtual std::size_t find_function_index(std::size_t module_index, const char* name) const override {
				try {
					return this->mediator->find_function_index(module_index, name);
				}
				catch (const std::out_of_range&) {
					return function_not_found;
				}
			}
			virtual std::size_t find_module_index(const char* name) const override {
				return this->mediator->find_module_index(name);
			}
			virtual return_value call_module(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string) override {
				try {
					return this->mediator->call_module(module_index, function_index, arguments_string);
				}
				catch ([[maybe_unused]] const std::exception& exc) {
					LOG_ERROR(this, exc.what());
					LOG_FATAL(this, "call_module has failed. The process will be terminated.");

					std::terminate();
				}
			}
		virtual return_value call_module_visible_only(std::size_t module_index, std::size_t function_index, arguments_string_type arguments_string, void(*error_callback)(call_error)) override {
            // ReSharper disable once CppInitializedValueIsAlwaysRewritten
            call_error error{ call_error::no_error };

			try {
				return this->mediator->call_module_visible_only(module_index, function_index, arguments_string);
			}
			catch (const exceptions::function_not_visible&) {
				error = call_error::function_is_not_visible;
			}
			catch (const std::out_of_range&) {
				error = call_error::unknown_index;
			}
			catch (const exceptions::invalid_arguments_string&) {
				error = call_error::invalid_arguments_string;
			}

			error_callback(error);
			return 0;
		}
	};

		std::vector<parser::components::engine_module> loaded_modules;
		module_part_implementation* part_implementation;

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
			:part_implementation{ new module_part_implementation{ this } }
		{
		}

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
			return this->part_implementation;
		}

		~engine_module_mediator() {
			delete this->part_implementation;
		}
	};
}

#endif
