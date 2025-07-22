#ifndef MODULE_FILE_COMPONENTS_H
#define MODULE_FILE_COMPONENTS_H

#include <string>
#include <Windows.h>
#include <iostream>

#include "module_part.h"

namespace module_mediator::parser::components {
	using module_callable_function_type = module_mediator::return_value(*)(module_mediator::arguments_string_type);
	class function {
	private:
		std::string name; //this function's name. used to identify its index
		module_mediator::arguments_string_type arguments_symbols; //a sequence of bytes that represent this function's arguments

		bool visible;
		module_callable_function_type function_address;

		void delete_arguments_symbols() {
			delete this->arguments_symbols;
			this->arguments_symbols = nullptr;
		}
		void move_value(function&& old_value) {
			this->arguments_symbols = old_value.arguments_symbols;
			this->name = std::move(old_value.name);
			this->visible = old_value.visible;
			this->function_address = old_value.function_address;

			old_value.arguments_symbols = nullptr;
		}

		bool compare_arguments_strings_arguments_count(module_mediator::arguments_string_type arguments_symbols) const {
			return arguments_symbols[0] == this->arguments_symbols[0];
		}
		bool check_arguments_strings_arguments_types(module_mediator::arguments_string_type arguments_symbols) const {
			return std::memcmp(arguments_symbols, this->arguments_symbols, static_cast<std::size_t>(this->arguments_symbols[0]) + 1) == 0;
		}
	public:
		function()
			:arguments_symbols{ nullptr },
			visible{ false },
			function_address{ nullptr }
		{
		}

		function(std::string&& name, module_callable_function_type function_address, module_mediator::arguments_string_type arguments_symbols, bool visible)
			:name{ std::move(name) },
			arguments_symbols{ arguments_symbols },
			visible{ visible },
			function_address{ function_address }
		{
		}

		function(const function&) = delete; //this type is used with std::vector which does not generally require copy constructor (only with special functions)
		void operator= (const function&) = delete;

		function(function&& old_value) noexcept {
			this->move_value(std::move(old_value));
		}
		void operator= (function&& old_value) noexcept {
			this->delete_arguments_symbols();
			this->move_value(std::move(old_value));
		}

		bool compare_names(std::string_view name) const { return this->name == name; }
		bool compare_arguments_types(module_mediator::arguments_string_type arguments_symbols) const { //true if equal
			assert(arguments_symbols && "null pointer");
			if (this->arguments_symbols == nullptr) { //if function has no arguments symbols it means that it automatically accepts all arguments
				return true;
			}

			if (this->compare_arguments_strings_arguments_count(arguments_symbols)) {
				return this->check_arguments_strings_arguments_types(arguments_symbols);
			}

			return false;
		}

		bool is_visible() const { return this->visible; }
		module_mediator::return_value call(module_mediator::arguments_string_type arguments) const {
			return this->function_address(arguments);
		}

		~function() {
			this->delete_arguments_symbols();
		}
	};

	class engine_module {
	private:
		std::string name; //used to find engine_module's index
		HMODULE loaded_module;

		std::vector<function> functions;

		void move_value(engine_module&& old_value) {
			this->name = std::move(old_value.name);
			this->functions = std::move(old_value.functions);

			this->loaded_module = old_value.loaded_module;
			old_value.loaded_module = NULL;
		}
		void free_resources() {
			this->free_module();
		}

		void load_module(const std::string& module_path) {
			this->loaded_module = LoadLibraryA(module_path.c_str());
			if (this->loaded_module == NULL) {
				std::cerr << "Unable to load one of the modules. Process will be terminated with std::abort."
					<< " (Path: " << module_path << ')' << std::endl;

				std::abort();
			}
		}
		void free_module() {
			if (this->loaded_module != NULL) {
				FARPROC free = GetProcAddress(this->loaded_module, "free_m");
				if (free != NULL) {
					((void(*)())free)();
				}

				BOOL freed_library = FreeLibrary(this->loaded_module);
				if (!freed_library) {
					std::cerr << "Unable to correctly dispose one of the modules. Process will be terminated with std::abort."
						<< " (Name: " << this->name << ')' << std::endl;

					std::abort();
				}
			}
		}
		void initialize_module(module_mediator::module_part* mediator) {
			FARPROC initialize = GetProcAddress(this->loaded_module, "initialize_m");
			if (initialize == NULL) {
				std::cerr << "One of the modules does not define the initialize_m function. Process will be terminated with std::abort."
					<< "(Name: " << this->name << ')' << std::endl;

				std::abort();
			}

			((void(*)(module_mediator::module_part*))initialize)(mediator); //convert and call initialize_m
		}
	public:
		engine_module(
			std::string&& module_name,
			std::string&& module_path,
			module_mediator::module_part* mediator //pointer to module_part allows to access some of the engine_module_mediator functions
		)
			:name{ std::move(module_name) },
			loaded_module{ NULL }
		{
			this->load_module(module_path);
			this->initialize_module(mediator);
		}

		engine_module(const engine_module&) = delete;
		void operator= (const engine_module&) = delete;

		engine_module(engine_module&& old_value) noexcept {
			this->move_value(std::move(old_value));
		}
		void operator= (engine_module&& old_value) noexcept {
			this->free_resources();
			this->move_value(std::move(old_value));
		}

		const std::string& get_name() const {
			return this->name;
		}

		bool compare_names(std::string_view name) const { return this->name == name; }
		std::size_t find_function_index(std::string_view name) const {
			for (std::size_t find_index = 0, size = this->functions.size(); find_index < size; ++find_index) {
				if (this->functions[find_index].compare_names(name)) {
					return find_index;
				}
			}

			return module_mediator::module_part::function_not_found;
		}

		const function& get_function(std::size_t index) const {
			return this->functions.at(index);
		}
		bool add_function(const std::string& name, std::string&& export_name, module_mediator::arguments_string_type arguments_string, bool is_visible) {
			FARPROC loaded_function = GetProcAddress(this->loaded_module, name.c_str());
			if (loaded_function == NULL)
				return false;

			this->functions.emplace_back(
                std::move(export_name),
					(module_callable_function_type)loaded_function,
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