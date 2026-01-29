#ifndef NDEBUG
//#include <vld.h>
#endif

#ifndef _MSC_VER
#error "Currently only MSVC is supported for the bytecode translator."
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <iostream>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <utility>
#include <sstream>
#include <iterator>
#include <vector>
#include <limits>

#include "parser_options.h"
#include "structure_builder.h"
#include "bytecode_translator.h"
#include "include_file_state.h"

#include "../generic_parser/parser_facade.h"
#include "../compression_algorithms/static_huffman.h"
#include "../compression_algorithms/sequence_reduction.h"

namespace {
    bool verify_program(const structure_builder::file& parser_value) {
        std::vector empty_functions{ check_functions_bodies(parser_value) };
        for (const std::string& name : empty_functions) {
            std::cout << "PROGRAM LOGIC WARNING: function with name '" + name + "' has empty body." << '\n';
        }

        std::cout << "Chosen stack size: " << parser_value.stack_size << " bytes." << '\n';
        if (parser_value.stack_size == 0) {
            std::cout << "PROGRAM LOGIC ERROR: You must explicitly specify the stack size that your program will use." <<
                '\n';
        }

        if (parser_value.main_function == nullptr) {
            std::cout << "SYNTAX ERROR: You must specify the starting function for your program." << '\n';
            return false;
        }

        if (!check_instructions_arguments(parser_value)) {
            std::cout << "SYNTAX ERROR: each instruction can have no more than " << max_instruction_arguments_count << " arguments." <<
                '\n';
            return false;
        }

        if (!check_functions_count(parser_value)) {
            std::cout << "SYNTAX ERROR: you can have no more than " << max_functions_count << " functions in one file." <<
                '\n';
            return false;
        }

        if (!check_functions_size(parser_value)) {
            std::cout << "SYNTAX ERROR: you can have no more than " << max_instructions_count << " instructions in each function." <<
                '\n';
            return false;
        }

        return true;
    }

    std::pair<std::stringstream, bool> produce_bytecode(structure_builder::file* parser_value, std::string debug_parameter) {
        std::stringstream result{ std::ios::in | std::ios::out | std::ios::binary };
        bytecode_translator translator{ parser_value, &result };

        if (debug_parameter == std::string{ "include-debug" }) {
            translator.start(true);
        }
        else if (debug_parameter == std::string{ "no-debug" }) {
            translator.start(false);
        }
        else {
            std::cout << "Unknown debug flag '" << debug_parameter << "' debug run won't be added.\n";
            translator.start(false);
        }

        for (translator_error_type err : translator.errors()) {
            std::cout << "PROGRAM LOGIC ERROR: ";
            translate_error(err, std::cout);

            std::cout << '\n';
        }

        return { std::move(result), !translator.errors().empty() };
    }

    std::vector<unsigned char> compress_bytecode(std::stringstream bytecode_stream) {
        constexpr unsigned short sequence_buffer_size = 256;

        std::vector<unsigned char> intermediate{};
        std::vector<unsigned char> result{ 'F', 'S', 'I' };

        std::copy_n(
            reinterpret_cast<const unsigned char*>(&sequence_buffer_size),
            sizeof(sequence_buffer_size),
            std::back_inserter(result)
        );

        compression_algorithms::static_huffman<std::size_t, unsigned char> huffman_compressor{};
        compression_algorithms::sequence_reduction sequence_reducer{ sequence_buffer_size };

        sequence_reducer.encode(
            std::istreambuf_iterator(bytecode_stream),
            std::istreambuf_iterator<char>{},
            std::back_inserter(intermediate)
        );

        huffman_compressor.create_encode_table(
            intermediate.begin(),
            intermediate.end()
        );

        std::size_t size_signature = intermediate.size();
        std::copy_n(
            reinterpret_cast<char*>(&size_signature),
            sizeof(size_signature),
            std::back_inserter(result)
        );

        for (std::size_t index = 0; index < huffman_compressor.get_encode_table_size(); ++index) {
            result.push_back(
                huffman_compressor.get_encode_object_symbol(index)
            );

            std::size_t symbol_count = huffman_compressor.get_encode_object_count(index);

            unsigned char bytes_needed = 0;
            std::size_t temp = symbol_count;
            do {
                bytes_needed++;
                temp >>= std::numeric_limits<unsigned char>::digits;
            } while (temp > 0);
            
            result.push_back(bytes_needed);
            std::copy_n(
                reinterpret_cast<char*>(&symbol_count),
                bytes_needed,
                std::back_inserter(result)
            );
        }

        huffman_compressor.encode(
            intermediate.begin(),
            intermediate.end(),
            std::back_inserter(result)
        );

        return result;
    }

    BOOL CtrlHandler(DWORD dwCtrlType) {
        if (dwCtrlType == CTRL_C_EVENT) {
            // Using EXIT_SUCCESS here is a questionable decision, because it indicates normal termination.
            // So just assume that the user wanted to terminate the program without errors.
            std::quick_exit(EXIT_SUCCESS);
        }

        return FALSE;
    }
}

// TODO: Use a wmain instead of a regular main to support Unicode file paths on Windows.

int main(int argc, char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();
    if (argc != 4) {
        std::cout << "Provide the name of the file to compile and its output destination. And add 'include-debug' or 'no-debug' at the end. It is only three arguments." <<
            '\n';
        return EXIT_FAILURE;
    }

    try {
        if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
            std::cout << "Failed to set control handler. This may impede user experience in fsi-visual-environment." <<
                '\n';
        }

        generic_parser::parser_facade<
            source_file_token,
            structure_builder::context_key,
            structure_builder
        > parser
        {
            parser_options::keywords,
            parser_options::contexts,
            source_file_token::name,
            source_file_token::end_of_file,
            structure_builder::context_key::main_context
        };

        std::filesystem::path main_program_file{ std::filesystem::canonical(argv[1]) };
        std::cout << "Now parsing: " << main_program_file.generic_string() << '\n';

        include_file_state::active_parsing_files.push_back(main_program_file);
        parser.start(main_program_file);

        std::pair error{ parser.error() };
        structure_builder::file parser_value{ parser.get_builder_value() };
        if (!error.second.empty()) {
            std::cout << "SYNTAX ERROR:	"
                << error.second.c_str()
                << " NEAR LINE " << error.first << '\n';

            return EXIT_FAILURE;
        }

        include_file_state::active_parsing_files.pop_back();
        if (!verify_program(parser_value)) {
            return EXIT_FAILURE;
        }

        std::cout << "Now translating the program to bytecode..." << '\n';
        auto [translated_program, has_logic_errors] = produce_bytecode(&parser_value, argv[3]);

        translated_program.seekg(0, std::ios::end);
        std::streamoff bytecode_size = translated_program.tellg();

        std::cout << "Now compressing the bytecode..." << '\n';
        translated_program.seekg(0, std::ios::beg);
        std::vector<unsigned char> compressed_bytecode = compress_bytecode(
            std::move(translated_program)
        );

        std::cout << "Compression ratio: "
            << static_cast<double>(compressed_bytecode.size()) / static_cast<double>(bytecode_size)
            << '.'
            << '\n';

        std::ofstream file_stream{ argv[2], std::ios::binary | std::ios::out };
        file_stream.write(
            reinterpret_cast<const char*>(compressed_bytecode.data()),
            static_cast<std::streamsize>(compressed_bytecode.size())
        );

        auto end_time = std::chrono::high_resolution_clock::now();
        std::cout << "Estimated time: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()
            << " microseconds."
            << '\n';

        if (has_logic_errors) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& exc) {
        std::cout << "Unable to process the file: " << exc.what() << '\n';
    }

    return EXIT_FAILURE;
}
