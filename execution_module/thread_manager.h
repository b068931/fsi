#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "scheduler.h"
#include "module_interoperation.h"
#include "assembly_functions.h"
#include "../logger_module/logging.h"

class thread_manager {
private:
	/*
	* it should be noted that scheduler works ONLY with unique ids,
	* and resource_module may reuse id after object with specific id was deleted.
	* this means that we must delete an object in this order:
	* 1. delete from scheduler
	* 2. delete from resource_manager
	*/

	scheduler scheduler;
	std::atomic_size_t active_threads_counter = 0;

	void executor_thread(module_mediator::return_value executor_id) {
		LOG_INFO(get_module_part(), std::format("Executor {} is starting.", executor_id));

		thread_local_structure* thread_structure = get_thread_local_structure();
		scheduler::schedule_information* currently_running_thread_information = 
			&(thread_structure->currently_running_thread_information);

		while (true) {
			bool choose_result = this->scheduler.choose(currently_running_thread_information);
			if (!choose_result) {
				LOG_INFO(get_module_part(), std::format("Executor {} is shutting down.", executor_id));
				break;
			}

			this->active_threads_counter.fetch_add(1, std::memory_order_relaxed); //all other modifications are synchronized with mutexes. this is one just needs atomicity
			load_program(
				thread_structure->execution_thread_state, 
				currently_running_thread_information->thread_state,
				(currently_running_thread_information->state == scheduler::thread_states::startup)
					? 1 : 0
			);

			size_t previous_active_threads_count = this->active_threads_counter.fetch_sub(1, std::memory_order_relaxed);
			if (currently_running_thread_information->put_back_structure) {
				this->scheduler.put_back(currently_running_thread_information->put_back_structure);
			}
			else {
				if ((previous_active_threads_count == 1) && (!this->scheduler.has_available_jobs())) {
					LOG_INFO(get_module_part(), "No available jobs found. Initiating shutdown."); //for this LOG_INFO the order is very important: it must be before the scheduler.initiate_shutdown() call
					this->scheduler.initiate_shutdown();
				}
			}
		}
	}

public:
	void add_thread_group(module_mediator::return_value id, std::uint64_t preferred_stack_size) {
		this->scheduler.add_thread_group(id, preferred_stack_size);
	}
	void add_thread(
		module_mediator::return_value thread_group_id,
		module_mediator::return_value thread_id,
		module_mediator::return_value priority,
		void* thread_state,
		const void* jump_table
	) {
		this->scheduler.add_thread(thread_group_id, thread_id, priority, thread_state, jump_table);
	}
	void forget_thread_group(module_mediator::return_value thread_group_id) {
		this->scheduler.forget_thread_group(thread_group_id);
	}
	bool delete_thread(module_mediator::return_value thread_group_id, module_mediator::return_value thread_id) {
		return this->scheduler.delete_thread(thread_group_id, thread_id);
	}
	void block(module_mediator::return_value thread_id) {
		this->scheduler.block(thread_id);
	}
	void make_runnable(module_mediator::return_value thread_id) {
		this->scheduler.make_runnable(thread_id);
	}
	void startup(std::uint16_t thread_count) {
		if(!this->scheduler.has_available_jobs()) {
			LOG_WARNING(get_module_part(), "No available jobs found. Executors won't start.");
			return;
		}

		std::vector<std::thread> executors{};
		for (std::uint16_t counter = 0; counter < thread_count; ++counter) {
			executors.emplace_back(&thread_manager::executor_thread, this, counter);
		}

		for (std::thread& executor : executors) {
			executor.join();
		}
	}
};

#endif