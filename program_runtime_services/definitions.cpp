#include "pch.h"
#include "declarations.h"

module_mediator::return_value get_current_thread_group_id() {
	return module_mediator::fast_call(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_get_current_thread_group_id()
	);
}
std::pair<void*, std::uint64_t> decay_pointer(module_mediator::pointer value) {
	if (value == nullptr) 
		return { nullptr, 0 };

	char* pointer = static_cast<char*>(value);
	void* data = nullptr;
	std::uint64_t size{};

	std::memcpy(&size, pointer, sizeof(std::uint64_t));
	std::memcpy(&data, pointer + sizeof(std::uint64_t), sizeof(std::uint64_t));

	return { data, size };
}