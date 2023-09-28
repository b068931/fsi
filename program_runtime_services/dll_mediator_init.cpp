#include "pch.h"
#include "declarations.h"
#include "dll_mediator_declarations.h"

dll_part* part = nullptr;
dll_part* get_dll_part() {
	return ::part;
}

void initialize_m(dll_part* part) {
	::part = part;
}