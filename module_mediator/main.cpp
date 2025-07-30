#ifndef NDEBUG
//#include <vld.h>
#endif

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <format>

#include "fsi_types.h"
#include "module_mediator.h"

#include "../logger_module/logging.h"
#include "../compression_algorithms/static_huffman.h"
#include "../compression_algorithms/sequence_reduction.h"

namespace {
    // Decompresses the bytecode from memory and stores that in a temporary file.
    std::filesystem::path decompress_bytecode(std::vector<unsigned char> compressed_data) {
        constexpr char header[]{ 'F', 'S', 'I' };

        std::vector<unsigned char>::iterator compressed_iterator(compressed_data.begin());
        if (compressed_data.size() < std::size(header) || !std::equal(std::begin(header), std::end(header), compressed_iterator)) {
            throw std::runtime_error("Invalid file signature.");
        }
        else {
            compressed_iterator += std::size(header);
        }

        unsigned short sequence_buffer_size = 0;
        if (compressed_iterator + sizeof(sequence_buffer_size) <= compressed_data.end()) {
            std::copy_n(
                compressed_iterator,
                sizeof(sequence_buffer_size),
                reinterpret_cast<unsigned char*>(&sequence_buffer_size)
            );

            compressed_iterator += sizeof(sequence_buffer_size);
        }
        else {
            throw std::runtime_error("Invalid compressed data: insufficient size for sequence buffer size.");
        }

        // Extract intermediate size (size after huffman decompression)
        std::size_t intermediate_size = 0;
        if (compressed_iterator + sizeof(intermediate_size) <= compressed_data.end()) {
            std::copy_n(
                compressed_iterator,
                sizeof(intermediate_size), 
                reinterpret_cast<unsigned char*>(&intermediate_size)
            );

            compressed_iterator += sizeof(intermediate_size);
        }
        else {
            throw std::runtime_error("Invalid compressed data: insufficient size for intermediate size.");
        }

        // Track total frequency to determine when we've parsed all symbols
        std::size_t total_frequency = 0;
        compression_algorithms::static_huffman<std::size_t, unsigned char> huffman_decompressor{};
        while (compressed_iterator != compressed_data.end() && total_frequency < intermediate_size) {
            unsigned char symbol = *compressed_iterator++;
            unsigned char bytes_needed = *compressed_iterator++;
            
            std::size_t symbol_count = 0;
            std::copy_n(
                compressed_iterator, 
                bytes_needed, 
                reinterpret_cast<unsigned char*>(&symbol_count)
            );

            huffman_decompressor.add_decode_object(
                symbol_count, 
                symbol, 
                huffman_decompressor.get_decode_table_size()
            );
            
            total_frequency += symbol_count;
            compressed_iterator += bytes_needed;
        }
        
        std::vector<unsigned char> intermediate;
        huffman_decompressor.decode(
            compressed_iterator,
            compressed_data.end(),
            intermediate_size,
            std::back_inserter(intermediate)
        );

        std::string temporary_file_name = std::format(
            "fsi-{}.tmp", 
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count()
        );

        std::filesystem::path temporary_file_path = std::filesystem::temp_directory_path() / temporary_file_name;
        std::ofstream temporary_file_stream(temporary_file_path, std::ios::binary | std::ios::out);
        if (!temporary_file_stream) {
            throw std::runtime_error("Failed to create temporary file.");
        }

        compression_algorithms::sequence_reduction sequence_reducer{ sequence_buffer_size };
        sequence_reducer.decode(
            intermediate.begin(),
            intermediate.end(),
            std::ostreambuf_iterator(temporary_file_stream)
        );

        if (intermediate.size() != intermediate_size) {
            throw std::runtime_error("Decompressed data size does not match expected intermediate size.");
        }
        
        return temporary_file_path;
    }

    std::vector<unsigned char> consume_compressed_bytecode(const std::string& file_name) {
        std::vector<unsigned char> compressed_bytecode{};
        std::ifstream compressed_stream{ file_name, std::ios::binary | std::ios::in };
        if (!compressed_stream) {
            throw std::runtime_error{
                std::format("Unable to open: {}.", file_name)
            };
        }

        compressed_stream.seekg(0, std::ios::end);
        std::size_t compressed_size = compressed_stream.tellg();
        compressed_stream.seekg(0, std::ios::beg);

        compressed_bytecode.resize(compressed_size);
        compressed_stream.read(
            reinterpret_cast<char*>(compressed_bytecode.data()), 
            static_cast<std::streamsize>(compressed_size)
        );

        return compressed_bytecode;
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "You need to provide three arguments: text file with the modules descriptions, executors count and a compiled file." << std::endl;
        return EXIT_FAILURE;
    }

    try {
        std::filesystem::path modules_descriptor_file{ std::filesystem::canonical(argv[1]) };
        std::cerr << "Parsing modules descriptor file: " << modules_descriptor_file.generic_string() << '\n';

        module_mediator::engine_module_mediator module_mediator{};
        std::string error_message = module_mediator.load_modules(modules_descriptor_file);

        module_mediator::module_part* part = module_mediator.get_module_part();
        if (error_message.empty()) {
            std::cerr << "All modules were loaded successfully." << '\n';
            std::uint16_t executors_count = static_cast<std::uint16_t>(std::stoi(argv[2])); //no point in starting an app if unable to parse the executors count

            std::filesystem::path decompressed_bytecode = decompress_bytecode(
                consume_compressed_bytecode(argv[3])
            );

            LOG_PROGRAM_INFO(
                part,
                std::format(
                    "Decompressed bytecode '{}' into file: '{}'.",
                    argv[3],
                    decompressed_bytecode.generic_string()
                )
            );

            std::size_t program_loader = part->find_module_index("progload");
            std::size_t load_program_to_memory = part->find_function_index(program_loader, "load_program_to_memory");

            std::string decompressed_bytecode_file_name = decompressed_bytecode.generic_string();
            module_mediator::fast_call<module_mediator::memory>(
                part, program_loader, load_program_to_memory,
                const_cast<char*>(decompressed_bytecode_file_name.c_str()) // May God forgive me for this
            );

            // Remove the temporary file after loading the program into memory.
            // Windows may use user's directory as a temporary directory, so we need to be sure that we are not leaving any garbage behind.
            std::filesystem::remove(decompressed_bytecode);

            // Attach to stdio does not manage stderr. That is done by the logger_module.
            std::size_t program_runtime_services = part->find_module_index("prts");
            std::size_t attach_to_stdio = part->find_function_index(program_runtime_services, "attach_to_stdio");
            module_mediator::fast_call(
                part, program_runtime_services, attach_to_stdio
            );

            std::size_t execution_module = part->find_module_index("excm");
            std::size_t startup = part->find_function_index(execution_module, "start");
            module_mediator::fast_call<module_mediator::two_bytes>(
                part, execution_module, startup,
                executors_count
            );

            std::size_t detach_from_stdio = part->find_function_index(program_runtime_services, "detach_from_stdio");
            module_mediator::fast_call(
                part, program_runtime_services, detach_from_stdio
            );
        }
        else {
            std::cout << "Could not correctly parse " << modules_descriptor_file.generic_string() << ": " << error_message << std::endl;
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& exc) {
        std::cout << "Was unable to start the engine: " << exc.what() << std::endl;
    }

    return EXIT_FAILURE;
}