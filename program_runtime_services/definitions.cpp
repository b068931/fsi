#include "pch.h"
#include "declarations.h"

return_value get_current_thread_group_id() {
	return fast_call_module(
		get_dll_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_group_id()
	);
}
std::pair<void*, uint64_t> decay_pointer(pointer value) {
	char* pointer = static_cast<char*>(value);
	void* data = nullptr;
	uint64_t size{};

	std::memcpy(&size, pointer, sizeof(uint64_t));
	std::memcpy(&data, pointer + sizeof(uint64_t), sizeof(uint64_t));

	return { data, size };
}