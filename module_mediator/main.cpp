#ifndef NDEBUG
//#include <vld.h>
#endif

#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "fsi_types.h"
#include "module_mediator.h"

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

			std::size_t progload = part->find_module_index("progload");
			std::size_t load_program_to_memory = part->find_function_index(progload, "load_program_to_memory");
			module_mediator::fast_call<module_mediator::memory>(
				part, progload, load_program_to_memory,
				argv[3]
			);

			// Attach to stdio does not manage stderr. That is done by the logger_module.
            std::size_t prts = part->find_module_index("prts");
            std::size_t attach_to_stdio = part->find_function_index(prts, "attach_to_stdio");
			module_mediator::fast_call(
				part, prts, attach_to_stdio
            );

			std::size_t excm = part->find_module_index("excm");
			std::size_t startup = part->find_function_index(excm, "start");
			module_mediator::fast_call<::module_mediator::two_bytes>(
				part, excm, startup,
				executors_count
			);

            std::size_t detach_from_stdio = part->find_function_index(prts, "detach_from_stdio");
			module_mediator::fast_call(
				part, prts, detach_from_stdio
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