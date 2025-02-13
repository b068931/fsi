#include <iostream>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <utility>

#include "parser_options.h"
#include "structure_builder.h"
#include "bytecode_translator.h"
#include "include_file_state.h"
#include "../generic_parser/parser_facade.h"

bool verify_program(const structure_builder::file& parser_value) {
	std::vector<std::string> empty_functions{ check_functions_bodies(parser_value) };
	for (const std::string& name : empty_functions) {
		std::cout << "PROGRAM LOGIC WARNING: function with name '" + name + "' has empty body." << std::endl;
	}

	std::cout << "Chosen stack size: " << parser_value.stack_size << " bytes." << std::endl;
	if (parser_value.stack_size == 0) {
		std::cout << "PROGRAM LOGIC ERROR: You must explicitly specify the stack size that your program will use." << std::endl;
	}

	if (parser_value.main_function == nullptr) {
		std::cout << "SYNTAX ERROR: You must specify the starting function for your program." << std::endl;
		return false;
	}

	if (!check_instructions_arugments(parser_value)) {
		std::cout << "SYNTAX ERROR: each instruction can have no more than " << max_instruction_arguments_count << " arguments." << std::endl;
		return false;
	}
	else if (!check_functions_count(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_functions_count << " functions in one file." << std::endl;
		return false;
	}
	else if (!check_functions_size(parser_value)) {
		std::cout << "SYNTAX ERROR: you can have no more than " << max_instructions_count << " instructions in each function." << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char** argv) {
	auto start_time = std::chrono::high_resolution_clock::now();
	if (argc != 4) {
		std::cout << "Provide the name of the file to compile and its output destination. And add 'include-debug' or 'no-debug' at the end. It is only three arguments." << std::endl;

		std::cin.get();
		return EXIT_FAILURE;
	}

	try {
		generic_parser::parser_facade<
			structure_builder::source_file_token,
			structure_builder::context_key,
			structure_builder
		> parser
		{
			parser_options::keywords,
			parser_options::contexts,
			structure_builder::source_file_token::name,
			structure_builder::source_file_token::end_of_file,
			structure_builder::context_key::main_context
		};

		std::filesystem::path main_program_file{ std::filesystem::canonical(argv[1]) };
		std::cout << "Now parsing: " << main_program_file.generic_string() << std::endl;

		include_file_state::active_parsing_files.push_back(main_program_file);
		parser.start(main_program_file);

		std::pair<structure_builder::line_type, std::string> error{ parser.error() };
		structure_builder::file parser_value{ parser.get_builder_value() };
		if (!error.second.empty()) {
			std::cout << "SYNTAX ERROR:	"
				<< error.second.c_str()
				<< " NEAR LINE " << error.first << std::endl;

			std::cin.get();
			return EXIT_FAILURE;
		}

		include_file_state::active_parsing_files.pop_back();
		if (!verify_program(parser_value)) {
			std::cin.get();
			return EXIT_FAILURE;
		}

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

		for (bytecode_translator::error_type err : translator.errors()) {
			std::cout << "PROGRAM LOGIC ERROR: ";
			translate_error(err, std::cout);

			std::cout << std::endl;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		std::cout << "Estimated time: "
			<< std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()
			<< " microseconds or "
			<< std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count()
			<< " seconds."
			<< std::endl;

		std::cin.get();
		return EXIT_SUCCESS;
	}
	catch (const std::exception& exc) {
		std::cout << "Unable to process the file: " << exc.what() << std::endl;
	}

	std::cin.get();
	return EXIT_FAILURE;
}