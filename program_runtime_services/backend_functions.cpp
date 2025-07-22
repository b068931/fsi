#include "pch.h"
#include "backend_functions.h"

namespace backend {
	std::pair<void*, std::uint64_t> decay_pointer(module_mediator::memory value) {
		if (value == nullptr)
			return { nullptr, 0 };

		char* pointer = static_cast<char*>(value);
		void* data = nullptr;
		std::uint64_t size{};

		std::memcpy(&size, pointer, sizeof(std::uint64_t));
		std::memcpy(static_cast<void*>(&data), pointer + sizeof(std::uint64_t), sizeof(std::uint64_t));

		return { data, size };
	}

	module_mediator::memory allocate_program_memory(module_mediator::eight_bytes size) {
		module_mediator::return_value null_pointer = reinterpret_cast<module_mediator::return_value>(nullptr);
		module_mediator::return_value allocated_memory = interoperation::allocate(size);

		if (allocated_memory == null_pointer) {
			return nullptr;
		}

		module_mediator::return_value value_pointer_data = interoperation::allocate(sizeof(std::uint64_t) * 2); //first 8 bytes - allocated size, second 8 bytes - base address
		if (value_pointer_data == null_pointer) {
            interoperation::deallocate(reinterpret_cast<module_mediator::memory>(allocated_memory));
			return nullptr;
		}

		char* pointer_data = reinterpret_cast<char*>(value_pointer_data);

		std::memcpy(pointer_data, &size, sizeof(std::uint64_t)); //fill in memory size
		std::memcpy(pointer_data + sizeof(std::uint64_t), &allocated_memory, sizeof(std::uint64_t)); //fill in actual memory value

		return pointer_data;
	}

	void deallocate_program_memory(module_mediator::memory address) {
		module_mediator::memory base{}; //save memory's base address
		std::memcpy(static_cast<void*>(&base), static_cast<char*>(address) + sizeof(std::uint64_t), sizeof(std::uint64_t));

		if (base == nullptr) return;

        interoperation::deallocate(address);
        interoperation::deallocate(base);
	}
}
