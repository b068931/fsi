#include <iostream>
#include <chrono>

#include "structure_builder.h"
#include "bytecode_translator.h"
#include "../generic_parser/parser_facade.h"

int main(int argc, char** argv) {
	auto start_time = std::chrono::high_resolution_clock::now();
	if (argc != 4) {
		std::cout << "Provide the name of the file to compile and its output destination. And add 'include-debug' or 'no-debug' at the end. It is only three arguments." << std::endl;

		std::cin.get();
		return 1;
	}

	generic_parser::parser_facade<structure_builder::source_file_token, structure_builder::context_key, structure_builder> parser
	{
		{
			{ "from", structure_builder::source_file_token::from_keyword },
			{ "import", structure_builder::source_file_token::import_keyword },
			{ "function", structure_builder::source_file_token::function_declaration_keyword },

			{ "redefine", structure_builder::source_file_token::redefine_keyword },
			{ "define", structure_builder::source_file_token::define_keyword },
			{ "undefine", structure_builder::source_file_token::undefine_keyword},
			{ "if-defined", structure_builder::source_file_token::if_defined_keyword },
			{ "if-not-defined", structure_builder::source_file_token::if_not_defined_keyword },
			{ "end-if", structure_builder::source_file_token::endif_keyword },
			{ "declare", structure_builder::source_file_token::declare_keyword },
			{ "stack-size", structure_builder::source_file_token::stack_size_keyword },
			{ "define-string", structure_builder::source_file_token::define_string_keyword },

			{ "immediate", structure_builder::source_file_token::immediate_argument_keyword },
			{ "signed", structure_builder::source_file_token::signed_argument_keyword },
			{ "variable", structure_builder::source_file_token::variable_argument_keyword },
			{ "dereference", structure_builder::source_file_token::pointer_dereference_argument_keyword },
			{ "function-name", structure_builder::source_file_token::function_address_argument_keyword },
			{ "point", structure_builder::source_file_token::jump_point_argument_keyword },
			{ "string", structure_builder::source_file_token::string_argument_keyword },
			{ "one-byte", structure_builder::source_file_token::one_byte_type_keyword },
			{ "two-bytes", structure_builder::source_file_token::two_bytes_type_keyword },
			{ "four-bytes", structure_builder::source_file_token::four_bytes_type_keyword },
			{ "eight-bytes", structure_builder::source_file_token::eight_bytes_type_keyword },
			{ "pointer", structure_builder::source_file_token::pointer_type_keyword },
			{ "size-of", structure_builder::source_file_token::sizeof_argument_keyword },

			{ "add", structure_builder::source_file_token::add_instruction_keyword },
			{ "signed-add", structure_builder::source_file_token::signed_add_instruction_keyword },
			{ "multiply", structure_builder::source_file_token::multiply_instruction_keyword },
			{ "signed-multiply", structure_builder::source_file_token::signed_multiply_instruction_keyword },
			{ "subtract", structure_builder::source_file_token::subtract_instruction_keyword },
			{ "signed-subtract", structure_builder::source_file_token::signed_subtract_instruction_keyword },
			{ "divide", structure_builder::source_file_token::divide_instruction_keyword },
			{ "signed-divide", structure_builder::source_file_token::signed_divide_instruction_keyword },
			{ "compare", structure_builder::source_file_token::compare_instruction_keyword },
			{ "increment", structure_builder::source_file_token::increment_instruction_keyword },
			{ "decrement", structure_builder::source_file_token::decrement_instruction_keyword },
			{ "jump", structure_builder::source_file_token::jump_instruction_keyword },
			{ "jump-equal", structure_builder::source_file_token::jump_equal_instruction_keyword },
			{ "jump-not-equal", structure_builder::source_file_token::jump_not_equal_instruction_keyword },
			{ "jump-greater", structure_builder::source_file_token::jump_greater_instruction_keyword },
			{ "jump-greater-equal", structure_builder::source_file_token::jump_greater_equal_instruction_keyword },
			{ "jump-less", structure_builder::source_file_token::jump_less_instruction_keyword },
			{ "jump-less-equal", structure_builder::source_file_token::jump_less_equal_instruction_keyword },
			{ "jump-above", structure_builder::source_file_token::jump_above_instruction_keyword },
			{ "jump-above-equal", structure_builder::source_file_token::jump_above_equal_instruction_keyword },
			{ "jump-below", structure_builder::source_file_token::jump_below_instruction_keyword },
			{ "jump-below-equal", structure_builder::source_file_token::jump_below_equal_instruction_keyword },
			{ "move", structure_builder::source_file_token::move_instruction_keyword },
			{ "copy-string", structure_builder::source_file_token::copy_string_instruction_keyword },
			{ "get-function-address", structure_builder::source_file_token::get_function_address_instruction_keyword },
			{ "bit-and", structure_builder::source_file_token::bit_and_instruction_keyword },
			{ "bit-or", structure_builder::source_file_token::bit_or_instruction_keyword },
			{ "bit-xor", structure_builder::source_file_token::bit_xor_instruction_keyword },
			{ "bit-not", structure_builder::source_file_token::bit_not_instruction_keyword },
			{ "bit-shift-left", structure_builder::source_file_token::bit_shift_left_instruction_keyword },
			{ "bit-shift-right", structure_builder::source_file_token::bit_shift_right_instruction_keyword },
			{ "save-value", structure_builder::source_file_token::save_value_instruction_keyword },
			{ "load-value", structure_builder::source_file_token::load_value_instruction_keyword },
			{ "move-pointer", structure_builder::source_file_token::move_pointer_instruction_keyword },
			{ "void", structure_builder::source_file_token::no_return_module_call_keyword }
		},
		{
			{
				structure_builder::context_key::main_context,
				generic_parser::token_generator<structure_builder::source_file_token, structure_builder::context_key>::symbols_pair{
					{
						{"/*", structure_builder::source_file_token::comment_start},
						{"*/", structure_builder::source_file_token::comment_end},
						{",", structure_builder::source_file_token::coma},
						{"$", structure_builder::source_file_token::special_instruction},
						{"<", structure_builder::source_file_token::import_start},
						{">", structure_builder::source_file_token::import_end},
						{"(", structure_builder::source_file_token::function_args_start},
						{")", structure_builder::source_file_token::function_args_end},
						{"{", structure_builder::source_file_token::function_body_start},
						{"}", structure_builder::source_file_token::function_body_end},
						{"[", structure_builder::source_file_token::dereference_start},
						{"]", structure_builder::source_file_token::dereference_end},
						{";", structure_builder::source_file_token::expression_end},
						{":", structure_builder::source_file_token::module_return_value},
						{"@", structure_builder::source_file_token::jump_point},
						{"->", structure_builder::source_file_token::module_call},
						{"$endif;", structure_builder::source_file_token::endif_keyword},
						{"\r\n", structure_builder::source_file_token::new_line}, //despite the fact that new line is a hard symbol, it won't be passed to the generic_builder. it is used to find the line with invalid syntax
						{"\n", structure_builder::source_file_token::new_line},
						{"''''", structure_builder::source_file_token::string_separator}
					},
					{ " ", "\t" }
				}
			},
			{
				structure_builder::context_key::inside_string,
				generic_parser::token_generator<structure_builder::source_file_token, structure_builder::context_key>::symbols_pair{
					{
						{"''''", structure_builder::source_file_token::string_separator}
					},
					{}
				}
			}
		},
		structure_builder::source_file_token::name,
		structure_builder::source_file_token::end_of_file,
		structure_builder::context_key::main_context
	};

	try {
		parser.start(argv[1]);
	}
	catch (const std::exception& exception) {
		std::cout << "Unable to process the file. The following exception might contain some information (or useless gibberish depending on your luck): " << exception.what() << std::endl;

		std::cin.get();
		return 1;
	}

	auto error = parser.error();
	auto parser_value = parser.get_builder_value();
	if (!error.second.empty()) {
		std::cout << "SYNTAX ERROR:	";
		std::cout << error.second << std::endl;
		std::cout << " NEAR LINE " << error.first << std::endl;

		std::cin.get();
		return 1;
	}

	std::vector<std::string> empty_functions{ check_functions_bodies(parser_value) };
	for (const std::string& name : empty_functions) {
		std::cout << "PROGRAM LOGIC WARNING: function with name '" + name + "' has empty body." << std::endl;
	}

	std::cout << "Chosen stack size: " << parser_value.stack_size << " bytes." << std::endl;
	if (parser_value.stack_size == 0) {
		std::cout << "PROGRAM LOGIC ERROR: You must explicitly specify the stack size that your program will use." << std::endl;
	}

	if (!check_instructions_arugments(parser_value)) {
		std::cout << "SYNTAX ERROR: each instruction can have no more than " << max_instruction_arguments_count << " arguments" << std::endl;

		std::cin.get();
		return 1;
	}
	else if (!check_functions_count(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_functions_count << " functions in one file" << std::endl;

		std::cin.get();
		return 1;
	}
	else if (!check_functions_size(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_instructions_count << " instructions in each function" << std::endl;

		std::cin.get();
		return 1;
	}
	else {
		std::ofstream file_stream{ argv[2], std::ios::binary | std::ios::out };
		bytecode_translator translator{ &parser_value, &file_stream };

		if (argv[3] == std::string{ "include-debug" }) {
			translator.start(true);
		}
		else if (argv[3] == std::string{ "no-debug" }) {
			translator.start(false);
		}
		else {
			std::cout << "Unknown debug flag '" << argv[3] << "' debug run won't be added.\n";
			translator.start(false);
		}

		for (bytecode_translator::error_t err : translator.errors()) {
			std::cout << "PROGRAM LOGIC ERROR: ";
			translate_error(err, std::cout);

			std::cout << std::endl;
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	std::cout << "Estimated time: " 
		<< std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() 
		<< " microseconds or " 
		<< std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count()
		<< " seconds"
		<< std::endl;

	std::cin.get();
	return 0;
}