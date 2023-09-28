#ifndef GENERIC_PARSER_H
#define GENERIC_PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <cassert>
#include <filesystem>
#include <algorithm>
#include <stack>
#include <map>
#include "typename_array.h"
#include "block_reader.h"

template<typename token_type, typename context_key_type>
class token_generator {
public:
	struct symbols_pair {
		//hard symbols are symbols that will be converted to their corresponding tokens regardless of their position in a file
		std::map<std::string, token_type> hard_symbols;
		
		//separators are symbols that are used to separate different names, separators can not be a part of a token
		std::vector<std::string> separators;
	};

private:
	block_reader<1024> reader;
	token_type name_token; //this token will be returned whenever name is found. names are symbols between separators and hard_symbols
	token_type end_token; //this token will be returned whenever end of file is encountered
	token_type additional_token; //additional token is created when hard symbol is encountered and it generated name from names_stack

	filepos name_start;
	filepos name_end;

	std::map<context_key_type, symbols_pair> symbols_list;
	typename std::map<context_key_type, symbols_pair>::iterator current_context;

	bool is_names_stack_token;

	//names stack is a structure that is shared between token_generator and builder. builder adds values to this structure to instantly convert name token to other token
	const std::vector<std::pair<std::string, token_type>>* names_stack;

	//found name will be stored in this variable
	std::string name;

	template<typename object, typename func> //compares multiple strings from container to one in the file
	auto multistring_find(const object& container, func get_string) { //get_string returns std::string based on object's iterator
		using iterator = typename object::const_iterator;

		std::vector<iterator> valid_hard_symbols{}; //create list of iterators that can potentially point to the best string
		for (auto begin = container.begin(), end = container.end(); begin != end; ++begin) {
			valid_hard_symbols.push_back(begin);
		}

		filepos saved_name_end = this->name_end; //we will restore this value after execution
		filepos multiindex = 0; //index in multiple strings

		iterator best_find = container.end();
		while (!valid_hard_symbols.empty()) {
			for (size_t current_string_index = 0; current_string_index < valid_hard_symbols.size(); ++current_string_index) {
				if (multiindex == get_string(valid_hard_symbols[current_string_index]).size()) { //if we reached the end of the string this means that this string is currently our best find. the longest string will be returned
					best_find = valid_hard_symbols[current_string_index];

					valid_hard_symbols.erase(valid_hard_symbols.begin() + current_string_index); //delete element and go 1 index back if needed
					--current_string_index;
				}
				else if (this->reader.get_symbol(this->name_end) != get_string(valid_hard_symbols[current_string_index])[multiindex]) { //if string in a file has different symbol
					valid_hard_symbols.erase(valid_hard_symbols.begin() + current_string_index);
					--current_string_index;
				}
			}

			++this->name_end;
			++multiindex;
		}

		this->name_end = saved_name_end;
		return best_find;
	}

	auto find_hard_symbols() { 
		return this->multistring_find(this->current_context->second.hard_symbols,
			[](typename decltype(this->current_context->second.hard_symbols)::const_iterator iterator) -> const std::string& {
				return iterator->first;
			}
		);
	}
	auto find_separators() {
		return this->multistring_find(this->current_context->second.separators,
			[](typename decltype(this->current_context->second.separators)::const_iterator iterator) -> const std::string& {
				return *iterator;
			}
		);
	}

	void create_name() {
		size_t name_size = this->name_end - this->name_start;
		if (name_size > 0) {
			this->name.resize(name_size);
			for (size_t name_index = 0; this->name_start < this->name_end; ++this->name_start, ++name_index) {
				this->name[name_index] = this->reader.get_symbol(this->name_start);
			}

			return;
		}

		this->name.resize(0); //if empty name was found in file, then empty name will be set here too
	}
	auto find_name_in_names_stack(const std::string& name) {
		return std::find_if(this->names_stack->crbegin(), this->names_stack->crend(), //find std::pair with specific name and return iterator
			[&name](const std::pair<std::string, token_type>& value) -> bool {
				return value.first == name;
			}
		);
	}
public:
	token_generator(
		std::map<context_key_type, symbols_pair>&& contexts, 
		const std::vector<std::pair<std::string, token_type>>* names_stack,
		token_type name_token,
		token_type end_token,
		context_key_type starting_context
	)
		:symbols_list{ std::move(contexts) },
		is_names_stack_token{ false },
		names_stack{ names_stack },
		reader{},
		name_token{ name_token },
		end_token{ end_token },
		additional_token{ end_token },
		name_start{ 0 },
		name_end{ 0 }
	{
		this->set_current_context(starting_context);
	}

	bool is_token_from_names_stack() const { return this->is_names_stack_token; }
	bool is_name_empty() const { return this->name.empty(); }
	token_type translate_string_through_names_stack(const std::string& name) {
		auto found_token = this->find_name_in_names_stack(name);
		if (found_token == this->names_stack->crend()) {
			return this->end_token;
		}

		return found_token->second;
	}

	token_type get_next_token() {
		this->is_names_stack_token = false;
		this->additional_token = this->end_token;
		while (this->name_end != this->reader.get_symbols_count()) {
			auto hard_symbol_iterator = this->find_hard_symbols();
			if (hard_symbol_iterator != this->current_context->second.hard_symbols.end()) {
				this->create_name(); //Names are created only after special symbols are encountered see(1)

				this->name_end += hard_symbol_iterator->first.size();
				this->name_start = this->name_end;

				auto found_name = this->find_name_in_names_stack(this->name);
				if (found_name != this->names_stack->crend()) {
					this->additional_token = found_name->second;
				}

				return hard_symbol_iterator->second;
			}

			auto separators_iterator = this->find_separators();
			if (separators_iterator != this->current_context->second.separators.end()) {
				this->create_name();

				this->name_end += separators_iterator->size();
				this->name_start = this->name_end;

				auto found_name = this->find_name_in_names_stack(this->name);
				if (found_name != this->names_stack->crend()) { //if name in names stack was found after separator was found, then we return associated token instead of name_token
					this->is_names_stack_token = true;
					return found_name->second;
				}

				return this->name_token;
			}

			++this->name_end; //move to the next symbol
		}

		this->create_name(); //1 or when end of file is encountered
		return this->end_token;
	}
	token_type get_additional_token() { return this->additional_token; }
	std::string&& get_name() { return std::move(this->name); }
	
	void set_current_context(context_key_type key) {
		auto found_context = this->symbols_list.find(key);
		assert(found_context != this->symbols_list.end() && "invalid key");

		this->current_context = found_context;
	}
	void reset_names() {
		this->name_start = 0;
		this->name_end = 0;
	}
	void open_file(const std::string& file_name) {
		this->reader.set_file_stream(new std::ifstream{ file_name, std::ios::binary | std::ios::in }, std::filesystem::file_size(file_name));
	}
};

template<typename token_type, typename context_key_type, typename builder>
class parser_facade {
private:
	//values are initialized in the order they appear in class, so generator and builder will both get valid pointers to this class
	std::vector<std::pair<std::string, token_type>> names_stack; //this field will be passed as parameter to both builder and token_generator, builder will use names_stack to get tokens instead of names
	token_type end_of_file;

	token_generator<typename token_type, context_key_type> generator;
	builder builder;
public:
	template<typename... builder_args> //names_stack, hard_symbols, separators, name_token, end_token, args that will be passed to initialize builder
	parser_facade(
		std::vector<std::pair<std::string, token_type>>&& names_stack,
		std::map<context_key_type, typename token_generator<token_type, context_key_type>::symbols_pair>&& contexts,
		token_type name_token,
		token_type end_token,
		context_key_type starting_context,
		builder_args&&... args
	)
		:names_stack{ std::move(names_stack) },
		generator{ std::move(contexts), &this->names_stack, name_token, end_token, starting_context },
		builder{ &this->names_stack, &this->generator, std::forward<builder_args>(args)... },
		end_of_file{ end_token }
	{}

	parser_facade(const parser_facade&) = delete;
	parser_facade(parser_facade&&) = delete;
	void operator= (const parser_facade&) = delete;
	void operator=(parser_facade&&) = delete;

	decltype(auto) error() { return this->builder.error(); }
	void start(const std::string& file_name) {
		this->generator.reset_names();
		this->generator.open_file(file_name);

		token_type token = this->end_of_file;
		do {
			token = generator.get_next_token();
			this->builder.handle_token(token);
		} while (this->builder.is_working());
	}
	decltype(auto) get_builder_value() { return this->builder.get_value(); } //auto will ignore const qualifier, references, etc. so this function returns decltype(auto)
};

#endif