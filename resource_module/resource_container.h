#ifndef RESOURCE_CONTAINER_H
#define RESOURCE_CONTAINER_H

#include "pch.h"
#include "id_generator.h"
#include "module_interoperation.h"
#include "../logger_module/logging.h"

struct resource_container {
	std::vector<std::pair<void(*)(void*), void*>> destroy_callbacks{};
	std::vector<void*> allocated_memory{};

	std::recursive_mutex* lock{ new std::recursive_mutex{} };

	resource_container() = default;
	void move_resource_container_to_this(resource_container&& object) {
		this->allocated_memory = std::move(object.allocated_memory);
	}

	resource_container(resource_container&& object) noexcept
		:allocated_memory{ std::move(object.allocated_memory) }
	{}
	void operator= (resource_container&& object) noexcept {
		this->move_resource_container_to_this(std::move(object));
	}

	virtual ~resource_container() noexcept {
		//calls to destroy callbacks must precede the deallocation of memory
		for (auto& callback : this->destroy_callbacks) {
			callback.first(callback.second);
		}

		if (!this->allocated_memory.empty()) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(), 
				std::format("Destroyed resource container had {} dangling memory block(s).", this->allocated_memory.size())
			);
		}

		for (void* memory : this->allocated_memory) {
			delete[] static_cast<char*>(memory);
		}

		delete this->lock;
	}
};

#endif