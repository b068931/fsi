#ifndef ENGINE_MODULE_MEDIATOR_H
#define ENGINE_MODULE_MEDIATOR_H

#define NOMINMAX 
#include <Windows.h>
#include <sstream>
#include <cassert>
#include <tuple>
#include <string_view>
#include <algorithm>
#include <stdexcept>

#include "module_part.h"
#include "../typename_array/typename_array.h"
#include "../generic_parser/token_generator.h"
#include "../generic_parser/parser_facade.h"
#include "../generic_parser/read_map.h"
#include "../console_and_debug/logging.h"

/*
* "When an evaluation of an expression writes to a memory location and another evaluation reads or modifies
* the same memory location, the expressions are said to conflict."
* find_module_index, find_function_index, call_module should not modify memory locations in any way
*/

using module_callable_function_type = module_mediator::return_value(*)(module_mediator::arguments_string_type);
class function {
private:
	std::string name; //this function's name. used to identify its index
	module_mediator::arguments_string_type arguments_symbols; //a sequence of bytes that represent this function's arguments

	bool visible;
	module_callable_function_type function_address;

	/*
		Structure:
		first byte - size of this string (including first byte and arguments' types, but excluding arguments' values)
		other bytes:
		0 = char,
		1 = uchar,
		2 = short,
		3 = ushort,
		4 = int,
		5 = uint,
		6 = long,
		7 = ulong,
		8 = llong,
		9 = ullong,
		10 = pointer

		Values of these arguments
	*/

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
		return std::memcmp(arguments_symbols, this->arguments_symbols, static_cast<size_t>(this->arguments_symbols[0]) + 1) == 0;
	}
public:
	function()
		:arguments_symbols{ nullptr },
		visible{ false },
		function_address{ nullptr }
	{}

	function(std::string&& name, module_callable_function_type function_address, module_mediator::arguments_string_type arguments_symbols, bool visible)
		:name{ std::move(name) },
		arguments_symbols{ arguments_symbols },
		visible{ visible },
		function_address{ function_address }
	{}

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
	size_t find_function_index(std::string_view name) const {
		for (size_t find_index = 0, size = this->functions.size(); find_index < size; ++find_index) {
			if (this->functions[find_index].compare_names(name)) {
				return find_index;
			}
		}

		return module_mediator::module_part::function_not_found;
	}
	
	const function& get_function(size_t index) const { 
		return this->functions.at(index); 
	}
	bool add_function(const std::string& name, std::string&& export_name, module_mediator::arguments_string_type arguments_string, bool is_visible) {
		FARPROC loaded_function = GetProcAddress(this->loaded_module, name.c_str());
		if (loaded_function == NULL)
			return false;

		this->functions.push_back(
			function{
				std::move(export_name),
				(module_callable_function_type)loaded_function, 
				arguments_string, 
				is_visible
			}
		);

		return true;
	}

	~engine_module() {
		this->free_resources();
	}
};

class engine_module_mediator;
class engine_module_builder {
	public:
		struct builder_parameters {
			module_mediator::module_part* module_part{};
			std::vector<std::string> arguments{ "char", "uchar", "short", "ushort", "int", "uint", "long", "ulong", "llong", "ullong", "pointer" };

			std::string module_name;

			bool is_visible{ false };
			std::string function_name;
			std::string function_exported_name;
		};

		enum class file_tokens {
			end_of_file,
			name, //this token is ignored, because configuration of token_generator for this class does not have base_separators
			new_line,
			header_open,
			header_close,
			value_assign,
			comment,
			name_and_public_name_separator,
			program_callable_function
		};
		enum class dynamic_parameters_keys {}; //unused
		enum class context_keys {
			main_context
		};

		using result_type = std::vector<engine_module>;
		using read_map_type = generic_parser::read_map<file_tokens, context_keys, result_type, builder_parameters, dynamic_parameters_keys>;
	private:
		result_type modules;
		builder_parameters parameters;

		generic_parser::token_generator<file_tokens, context_keys>* generator;
		std::vector<std::pair<std::string, engine_module_builder::file_tokens>>* names_stack;
		
		engine_module_mediator* mediator;

		read_map_type parse_map;
		void configure_parse_map();

	public:
		engine_module_builder(
			std::vector<std::pair<std::string, engine_module_builder::file_tokens>>* names_stack, 
			generic_parser::token_generator<engine_module_builder::file_tokens, context_keys>* token_generator, 
			engine_module_mediator* mediator
		) //"mediator" will be used to initialize engine_module objects
			:parse_map{file_tokens::end_of_file, file_tokens::name, token_generator},
			generator{token_generator},
			names_stack{names_stack},
			mediator{mediator}
		{
			this->configure_parse_map();
		}

		const std::string& error() { return this->parse_map.error(); }
		bool is_working() { return this->parse_map.is_working(); }
		void handle_token(engine_module_builder::file_tokens token) { 
			this->parse_map.handle_token(&this->modules, token, &this->parameters);
		}
		result_type get_value() { return std::move(this->modules); }
	};

class engine_module_mediator {
private:
	class module_part_implementation : public module_mediator::module_part {
	private:
		engine_module_mediator* mediator;

	public:
		module_part_implementation(engine_module_mediator* mediator)
			:mediator{ mediator }
		{}

		virtual size_t find_function_index(size_t module_index, const char* name) const override {
			try {
				return this->mediator->find_function_index(module_index, name);
			}
			catch (const std::out_of_range&) {
				return module_mediator::module_part::function_not_found;
			}
		}
		virtual size_t find_module_index(const char* name) const override {
			return this->mediator->find_module_index(name);
		}
		virtual module_mediator::return_value call_module(size_t module_index, size_t function_index, module_mediator::arguments_string_type arguments_string) override {
			try {
				return this->mediator->call_module(module_index, function_index, arguments_string);
			}
			catch (const std::exception& exc) {
				log_error(this, exc.what());
				log_fatal(this, "call_module has failed. The process will be terminated with 'abort'.");

				std::abort();
			}
		}
		virtual module_mediator::return_value call_module_visible_only(size_t module_index, size_t function_index, module_mediator::arguments_string_type arguments_string, void(*error_callback)(call_error)) override {
			call_error error{ call_error::no_error };
			
			try {
				return this->mediator->call_module_visible_only(module_index, function_index, arguments_string);
			}
			catch (const function_not_visible&) {
				error = call_error::function_is_not_visible;
			}
			catch (const std::out_of_range&) {
				error = call_error::unknown_index;
			}
			catch (const invalid_arguments_string&) {
				error = call_error::invalid_arguments_string;
			}

			error_callback(error);
			return 0;
		}
	};

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

	std::vector<engine_module> loaded_modules;
	module_part_implementation* part_implementation;

	const engine_module& get_module(size_t module_index) const {
		return this->loaded_modules.at(module_index);
	}

	size_t find_module_index(std::string_view name) const {
		for (size_t index = 0, size = this->loaded_modules.size(); index < size; ++index) {
			if (this->loaded_modules[index].compare_names(name)) {
				return index;
			}
		}

		return module_mediator::module_part::module_not_found;
	}
	size_t find_function_index(size_t module_index, std::string_view name) const { 
		return this->get_module(module_index).find_function_index(name);
	}

	module_mediator::return_value call_module(size_t module_index, size_t function_index, module_mediator::arguments_string_type arguments_string) {
		if (this->get_module(module_index).get_function(function_index).compare_arguments_types(arguments_string)) { //true if arguments match
			return this->get_module(module_index).get_function(function_index).call(arguments_string);
		}

		throw invalid_arguments_string{ "Invalid arguments string used." };
	}
	module_mediator::return_value call_module_visible_only(size_t module_index, size_t function_index, module_mediator::arguments_string_type arguments_string) {
		if (this->get_module(module_index).get_function(function_index).is_visible()) {
			return this->call_module(module_index, function_index, arguments_string);
		}

		throw function_not_visible{ "This function is not visible." };
	}
public:
	engine_module_mediator()
		:part_implementation{ new module_part_implementation{ this } }
	{}

	std::string load_modules(std::string file_name) {
		generic_parser::parser_facade<engine_module_builder::file_tokens, engine_module_builder::context_keys, engine_module_builder> parser{
			{},
			{
				{
					engine_module_builder::context_keys::main_context,
					generic_parser::token_generator<engine_module_builder::file_tokens, engine_module_builder::context_keys>::symbols_pair{
						{
							{":", engine_module_builder::file_tokens::name_and_public_name_separator},
							{"!", engine_module_builder::file_tokens::program_callable_function},
							{"--", engine_module_builder::file_tokens::comment},
							{"[", engine_module_builder::file_tokens::header_open},
							{"]", engine_module_builder::file_tokens::header_close},
							{"=", engine_module_builder::file_tokens::value_assign},
							{"\n", engine_module_builder::file_tokens::new_line},
							{"\r\n", engine_module_builder::file_tokens::new_line}
						},
						{}
					}
				}
			},
			engine_module_builder::file_tokens::name,
			engine_module_builder::file_tokens::end_of_file,
			engine_module_builder::context_keys::main_context,
			this
		};

		try {
			parser.start(file_name);
		}
		catch (const std::filesystem::filesystem_error&) {
			return std::string{ "Can not find 'dlls.txt', it is required in order to load appropriate modules." };
		}

		this->loaded_modules = parser.get_builder_value();
		return parser.error();
	}
	module_mediator::module_part* get_module_part() {
		return this->part_implementation;
	}

	~engine_module_mediator() {
		delete this->part_implementation;
	}
};

#endif