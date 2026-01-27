#ifndef THREAD_STRUCTURE_H
#define THREAD_STRUCTURE_H

#include "pch.h"
#include "resource_container.h"

struct thread_structure : public resource_container {
	std::size_t program_container{};

	thread_structure() = default;

	thread_structure(std::size_t program_container_id)
		:resource_container{},
		program_container{ program_container_id }
	{}

    thread_structure(const thread_structure& structure) = delete;
    thread_structure& operator= (const thread_structure& structure) = delete;

	thread_structure(thread_structure&& structure) noexcept
		:resource_container{ std::move(static_cast<resource_container&&>(structure)) },
		program_container{ structure.program_container }
	{
		structure.program_container = std::size_t{};
	}

	thread_structure& operator= (thread_structure&& structure) noexcept {
		this->move_resource_container_to_this(std::move(static_cast<resource_container&&>(structure)));
		this->program_container = structure.program_container;

		structure.program_container = std::size_t{};
		return *this;
	}

	~thread_structure() noexcept override = default;
};

#endif // !THREAD_STRUCTURE_H
