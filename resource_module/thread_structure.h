#ifndef THREAD_STRUCTURE_H
#define THREAD_STRUCTURE_H

#include "pch.h"
#include "resource_container.h"

struct thread_structure : public resource_container {
	std::size_t program_container{};

	thread_structure() = default;
	thread_structure(std::size_t program_container)
		:resource_container{},
		program_container{ program_container }
	{}

	thread_structure(thread_structure&& structure) noexcept
		:resource_container{ std::move(structure) },
		program_container{ structure.program_container }
	{
		structure.program_container = std::size_t{};
	}
	void operator= (thread_structure&& structure) noexcept {
		this->move_resource_container_to_this(std::move(structure));
		this->program_container = structure.program_container;

		structure.program_container = std::size_t{};
	}
};

#endif // !THREAD_STRUCTURE_H
