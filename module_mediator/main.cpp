#include <iostream>
#include <sstream>

#include "module_mediator.h"

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cout << "You need to provide two arguments: executors count and a compiled file." << std::endl;

		system("pause");
		return 1;
	}

	int executors_count = 0;
	std::stringstream converter{ argv[1] };

	converter >> executors_count;
	if (!converter.eof()) {
		std::cout << "The second argument must be an integer." << std::endl;

		system("pause");
		return 1;
	}

	std::cout << "Type 'okimdone' to exit. Make sure that your program is actually done executing." << std::endl;

	module_mediator::engine_module_mediator module_mediator{};
	std::string error_message = module_mediator.load_modules("dlls.txt");

	module_mediator::module_part* part = module_mediator.get_module_part();
	if (error_message.empty()) {

		std::size_t excm = part->find_module_index("excm");
		std::size_t startup = part->find_function_index(excm, "start");
		std::size_t run = part->find_function_index(excm, "run_program");

		std::unique_ptr<module_mediator::arguments_string_element[]> args{ module_mediator::arguments_string_builder::pack<void*>(argv[2]) };
		part->call_module(excm, run, args.get());

		args.reset(module_mediator::arguments_string_builder::pack<module_mediator::return_value>(executors_count));
		part->call_module(excm, startup, args.get());
	}
	else {
		std::cout << "Could not correctly parse dlls.txt: " << error_message << std::endl;
	}

	std::string user_input{};
	do {
		std::cin >> user_input;
	} while (user_input != "okimdone");

	return 0;
}