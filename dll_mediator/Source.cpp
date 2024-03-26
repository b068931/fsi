#include <iostream>
#include <sstream>

#include "dll_mediator.h"

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

	dll_mediator dll_mediator{};
	std::string error_message = dll_mediator.load_dlls("dlls.txt");

	dll_part* part = dll_mediator.get_dll_part();
	if (error_message.empty()) {

		size_t excm = part->find_dll_index("excm");
		size_t startup = part->find_function_index(excm, "start");
		size_t run = part->find_function_index(excm, "run_program");

		std::unique_ptr<arguments_string_element[]> args{ arguments_string_builder::pack<void*>(argv[2]) };
		part->call_module(excm, run, args.get());

		args.reset(arguments_string_builder::pack<return_value>(executors_count));
		part->call_module(excm, startup, args.get());
	}
	else {
		std::cout << error_message << std::endl;
	}

	std::string user_input{};
	do {
		std::cin >> user_input;
	} while (user_input != "okimdone");

	return 0;
}