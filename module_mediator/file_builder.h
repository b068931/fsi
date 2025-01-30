#ifndef MODULE_MEDIATOR_FILE_BUILDER_H
#define MODULE_MEDIATOR_FILE_BUILDER_H

#include <vector>
#include <string>

#include "module_part.h"
#include "file_components.h"
#include "../generic_parser/token_generator.h"
#include "../generic_parser/read_map.h"

namespace module_mediator {
	class engine_module_mediator;
}

namespace module_mediator::parser::components {
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
			:parse_map{ file_tokens::end_of_file, file_tokens::name, token_generator },
			generator{ token_generator },
			names_stack{ names_stack },
			mediator{ mediator }
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
}

#endif