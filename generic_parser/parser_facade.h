#ifndef PARSER_FACADE_H
#define PARSER_FACADE_H

#include <vector>
#include <utility>
#include <string>
#include <map>
#include <filesystem>

#include "token_generator.h"

namespace generic_parser {
	template<typename token_type, typename context_key_type, typename builder_type>
	class parser_facade {
	private:
		//values are initialized in the order they appear in class, so generator and builder will both get valid pointers to this class
		std::vector<std::pair<std::string, token_type>> names_stack; //this field will be passed as parameter to both builder and token_generator, builder will use names_stack to get tokens instead of names
		token_type end_of_file;

		token_generator<token_type, context_key_type> generator;
		builder_type builder;
	public:
		template<typename... builder_args> //names_stack, hard_symbols, separators, name_token, end_token, args that will be passed to initialize builder
		parser_facade(
			const std::vector<std::pair<std::string, token_type>>& names_stack,
			const std::map<context_key_type, typename token_generator<token_type, context_key_type>::symbols_pair>& contexts,
			token_type name_token,
			token_type end_token,
			context_key_type starting_context,
			builder_args&&... args
		)
			:names_stack{ names_stack },
			end_of_file{ end_token },
			generator{ contexts, &this->names_stack, name_token, end_token, starting_context },
			builder{ &this->names_stack, &this->generator, std::forward<builder_args>(args)... }
		{
		}

		parser_facade(const parser_facade&) = delete;
		parser_facade& operator= (const parser_facade&) = delete;

		parser_facade(parser_facade&&) = delete;
		parser_facade& operator=(parser_facade&&) = delete;

		decltype(auto) error() { return this->builder.error(); }
		void start(const std::filesystem::path& file_name) {
			this->generator.reset_names();
			this->generator.open_file(file_name);

			token_type token = this->end_of_file;
			do {
				token = generator.get_next_token();
				this->builder.handle_token(token);
			} while (this->builder.is_working());
		}

		decltype(auto) get_builder_value() { return this->builder.get_value(); } //auto will ignore const qualifier, references, etc. so this function returns decltype(auto)
		builder_type& get_builder() { return this->builder; }

		~parser_facade() noexcept = default;
	};
}

#endif