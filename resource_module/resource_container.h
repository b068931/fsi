#ifndef RESOURCE_CONTAINER_H
#define RESOURCE_CONTAINER_H

#include <vector>
#include <mutex>
#include <utility>

struct resource_container {
	std::vector<std::pair<void(*)(return_value, return_value, void*), void*>> destroy_callbacks{};
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

	~resource_container() noexcept {
		for (void* memory : this->allocated_memory) {
			delete[] memory;
		}

		delete this->lock;
	}
};

#endif