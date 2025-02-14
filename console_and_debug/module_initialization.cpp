#include "pch.h"
#include "console_and_debug.h"
#include "module_interoperation.h"
#include "../module_mediator/module_part.h"

module_mediator::module_part* part = nullptr;
extern std::chrono::steady_clock::time_point starting_time;

module_mediator::module_part* get_module_part() {
	return ::part;
}

void initialize_m(module_mediator::module_part* part) {
	::part = part;
	::starting_time = std::chrono::steady_clock::now();

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		std::cerr << "Unable to set virtual terminal mode. Output may look strange." << std::endl;
	}
}