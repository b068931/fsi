#ifndef PROGRAM_CONTAINER_H
#define PROGRAM_CONTAINER_H

#include "pch.h"
#include "resource_container.h"
#include "program_context.h"

struct program_container : public resource_container {
	size_t threads_count{ 0 };
	program_context* context{};

	program_container() = default;

	program_container(program_container&& container) noexcept
		:resource_container{ std::move(container) },
		context{ container.context },
		threads_count{ container.threads_count }
	{
		container.threads_count = 0;
		container.context = nullptr;
	}
	void operator=(program_container&& container) noexcept {
		this->move_resource_container_to_this(std::move(container));
		this->threads_count = container.threads_count;
		this->context = container.context;

		container.threads_count = 0;
		container.context = nullptr;
	}

	~program_container() noexcept;
};

#endif // !PROGRAM_CONTAINER_H