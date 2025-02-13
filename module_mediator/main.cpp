#include <iostream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <format>

#include "module_mediator.h"

int main(int argc, char** argv) {
	if (argc != 4) {
		std::cout << "You need to provide three arguments: text file with the modules descriptions, executors count and a compiled file." << std::endl;

		std::cin.get();
		return EXIT_FAILURE;
	}

	try {
		std::filesystem::path modules_descriptor_file{ std::filesystem::canonical(argv[1]) };
		std::cout << "Parsing modules descriptor file: " << modules_descriptor_file.generic_string() << std::endl;

		module_mediator::engine_module_mediator module_mediator{};
		std::string error_message = module_mediator.load_modules(modules_descriptor_file);

		module_mediator::module_part* part = module_mediator.get_module_part();
		if (error_message.empty()) {
			std::cout << "All modules were loaded successfully. Commencing program execution." << std::endl;

			std::size_t excm = part->find_module_index("excm");
			std::size_t startup = part->find_function_index(excm, "start");
			std::size_t run = part->find_function_index(excm, "run_program");

			std::string program_file_name{ std::filesystem::canonical(argv[3]).generic_string() };
			std::cout << "Starting program: " << program_file_name << std::endl;

			std::unique_ptr<module_mediator::arguments_string_element[]> args{
				module_mediator::arguments_string_builder::pack<void*>(program_file_name.data())
			};

			int executors_count = std::stoi(argv[2]); //no point in starting an app if unable to parse the executors count
			part->call_module(excm, run, args.get());

			args.reset(module_mediator::arguments_string_builder::pack<module_mediator::return_value>(executors_count));
			part->call_module(excm, startup, args.get());
		}
		else {
			std::cout << "Could not correctly parse " << modules_descriptor_file.generic_string() << ": " << error_message << std::endl;
		}

		std::cin.get();
		return EXIT_SUCCESS;
	}
	catch (const std::exception& exc) {
		std::cout << "Was unable to start the engine: " << exc.what() << std::endl;
	}

	return EXIT_FAILURE;
}