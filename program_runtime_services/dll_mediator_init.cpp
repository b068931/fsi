#include "pch.h"
#include "declarations.h"
#include "dll_mediator_declarations.h"

module_mediator::dll_part* part = nullptr;
module_mediator::dll_part* get_dll_part() {
	return ::part;
}

void initialize_m(module_mediator::dll_part* part) {
	::part = part;
}