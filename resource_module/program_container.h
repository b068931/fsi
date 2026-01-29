#ifndef PROGRAM_CONTAINER_H
#define PROGRAM_CONTAINER_H

#include "pch.h"
#include "resource_container.h"
#include "program_context.h"

struct program_container : resource_container {
	std::size_t threads_count{ 0 };
	program_context* context{};

	program_container() = default;

    program_container(const program_container& container) = delete;
    program_container& operator=(const program_container& container) = delete;

	program_container(program_container&& container) noexcept
		:resource_container{ std::move(static_cast<resource_container&&>(container)) },
		threads_count{ container.threads_count },
		context{ container.context }
	{
		container.threads_count = 0;
		container.context = nullptr;
	}

	program_container& operator=(program_container&& container) noexcept {
		this->move_resource_container_to_this(std::move(static_cast<resource_container&&>(container)));
		this->threads_count = container.threads_count;
		this->context = container.context;

		container.threads_count = 0;
		container.context = nullptr;

		return *this;
	}

	~program_container() noexcept override;
};

#endif // !PROGRAM_CONTAINER_H
