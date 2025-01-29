#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "pch.h"
#include "scheduler.h"
#include "functions.h"

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
	void executor_thread() {
		thread_local_structure* thread_structure = get_thread_local_structure();
		scheduler::schedule_information* currently_running_thread_information = 
			&(thread_structure->currently_running_thread_information);

		while (true) {
			this->scheduler.choose(currently_running_thread_information);

			load_programf(
				thread_structure->execution_thread_state, 
				currently_running_thread_information->thread_state,
				(currently_running_thread_information->state == scheduler::thread_states::startup)
					? 1 : 0
			);

			if (currently_running_thread_information->put_back_structure) {
				this->scheduler.put_back(currently_running_thread_information->put_back_structure);
			}
		}
	}

public:
	void add_thread_group(module_mediator::return_value id) {
		this->scheduler.add_thread_group(id);
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
	void startup(uint16_t thread_count) {
		for (uint16_t counter = 0; counter < thread_count; ++counter) {
			std::thread executor_daemon{ &thread_manager::executor_thread, this };
			executor_daemon.detach();
		}
	}
};

#endif