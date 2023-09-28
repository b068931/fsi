#include <iostream>
#include <chrono>
#include "structure_builder.h"
#include "bytecode_translator.h"

int main(int argc, char** argv) {
	auto start_time = std::chrono::high_resolution_clock::now();
	if (argc != 4) {
		std::cout << "Provide the name of the file to compile and its output destination. And add 'include-debug' or 'no-debug' at the end. It is only three arguments." << std::endl;
		return 1;
	}

	parser_facade<structure_builder::source_file_token, structure_builder::context_key, structure_builder> parser
	{
		{
			{ "function", structure_builder::source_file_token::function_declaration },
			{ "from", structure_builder::source_file_token::FROM },
			{ "import", structure_builder::source_file_token::IMPORT },
			{ "redefine", structure_builder::source_file_token::REDEFINE },
			{ "define", structure_builder::source_file_token::DEFINE },
			{ "undefine", structure_builder::source_file_token::UNDEFINE},
			{ "ifdef", structure_builder::source_file_token::IFDEF },
			{ "ifndef", structure_builder::source_file_token::IFNDEF },
			{ "endif", structure_builder::source_file_token::ENDIF },
			{ "stack_size", structure_builder::source_file_token::STACK_SIZE },
			{ "string", structure_builder::source_file_token::STRING },
			{ "byte", structure_builder::source_file_token::BYTE },
			{ "dbyte", structure_builder::source_file_token::DBYTE },
			{ "fbyte", structure_builder::source_file_token::FBYTE },
			{ "ebyte", structure_builder::source_file_token::EBYTE },
			{ "pointer", structure_builder::source_file_token::POINTER },
			{ "decl", structure_builder::source_file_token::DECL },
			{ "add", structure_builder::source_file_token::ADD },
			{ "sadd", structure_builder::source_file_token::SADD },
			{ "multiply", structure_builder::source_file_token::MULTIPLY },
			{ "smultiply", structure_builder::source_file_token::SMULTIPLY },
			{ "substract", structure_builder::source_file_token::SUBSTRACT },
			{ "ssubstract", structure_builder::source_file_token::SSUBSTRACT },
			{ "divide", structure_builder::source_file_token::DIVIDE },
			{ "sdivide", structure_builder::source_file_token::SDIVIDE },
			{ "compare", structure_builder::source_file_token::COMPARE },
			{ "increment", structure_builder::source_file_token::INCREMENT },
			{ "decrement", structure_builder::source_file_token::DECREMENT },
			{ "jump", structure_builder::source_file_token::JUMP },
			{ "jump_equal", structure_builder::source_file_token::JUMP_EQUAL },
			{ "jump_not_equal", structure_builder::source_file_token::JUMP_NOT_EQUAL },
			{ "jump_greater", structure_builder::source_file_token::JUMP_GREATER },
			{ "jump_greater_equal", structure_builder::source_file_token::JUMP_GREATER_EQUAL },
			{ "jump_less", structure_builder::source_file_token::JUMP_LESS },
			{ "jump_less_equal", structure_builder::source_file_token::JUMP_LESS_EQUAL },
			{ "jump_above", structure_builder::source_file_token::JUMP_ABOVE },
			{ "jump_above_equal", structure_builder::source_file_token::JUMP_ABOVE_EQUAL },
			{ "jump_below", structure_builder::source_file_token::JUMP_BELOW },
			{ "jump_below_equal", structure_builder::source_file_token::JUMP_BELOW_EQUAL },
			{ "move", structure_builder::source_file_token::MOVE },
			{ "imm", structure_builder::source_file_token::immediate_data },
			{ "signed", structure_builder::source_file_token::SIGNED },
			{ "var", structure_builder::source_file_token::variable_referenced },
			{ "ref", structure_builder::source_file_token::pointer_dereference },
			{ "fnc", structure_builder::source_file_token::function_address },
			{ "pnt", structure_builder::source_file_token::jump_data },
			{ "str", structure_builder::source_file_token::string_argument },
			{ "and", structure_builder::source_file_token::AND },
			{ "or", structure_builder::source_file_token::OR },
			{ "xor", structure_builder::source_file_token::XOR },
			{ "not", structure_builder::source_file_token::NOT },
			{ "void", structure_builder::source_file_token::VOID },
			{ "save", structure_builder::source_file_token::SAVE },
			{ "load", structure_builder::source_file_token::LOAD },
			{ "pset", structure_builder::source_file_token::REF },
			{ "shift_left", structure_builder::source_file_token::SHIFT_LEFT },
			{ "shift_right", structure_builder::source_file_token::SHIFT_RIGHT },
			{ "ctjtd", structure_builder::source_file_token::CTJTD },
			{ "copy_string", structure_builder::source_file_token::COPY_STRING },
			{ "sizeof", structure_builder::source_file_token::SIZEOF }
		},
		{
			{
				structure_builder::context_key::main_context,
				token_generator<structure_builder::source_file_token, structure_builder::context_key>::symbols_pair{
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
						{"$endif", structure_builder::source_file_token::ENDIF},
						{"\r\n", structure_builder::source_file_token::new_line}, //despite the fact that new line is a hard symbol, it won't be passed to the generic_builder. it is used to find the line with invalid syntax
						{"\n", structure_builder::source_file_token::new_line},
						{"''''", structure_builder::source_file_token::string_separator}
					},
					{ " ", "\t" }
				}
			},
			{
				structure_builder::context_key::inside_string,
				token_generator<structure_builder::source_file_token, structure_builder::context_key>::symbols_pair{
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
	catch (const std::exception&) {
		std::cout << "Unable to process the file." << std::endl;
		return 1;
	}

	auto error = parser.error();
	auto parser_value = parser.get_builder_value();
	if (!error.second.empty()) {
		std::cout << "SYNTAX ERROR:	";
		std::cout << error.second << std::endl;

		std::cout << " NEAR LINE " << error.first << std::endl;
		return 1;
	}

	std::vector<std::string> empty_functions{ check_functions_bodies(parser_value) };
	for (const std::string& name : empty_functions) {
		std::cout << "PROGRAM LOGIC WARNING: function with name '" + name + "' has empty body." << std::endl;
	}

	if (!check_instructions_arugments(parser_value)) {
		std::cout << "SYNTAX ERROR: each instruction can have no more than " << max_instruction_arguments_count << " arguments" << std::endl;
		return 1;
	}
	else if (!check_functions_count(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_functions_count << " functions in one file" << std::endl;
		return 1;
	}
	else if (!check_functions_size(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_instructions_count << " instructions in each function" << std::endl;
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
			std::cout << "Unknown debug flag '" << argv[3] << "' debug run won't be added.";
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

	system("pause");
	return 0;
}