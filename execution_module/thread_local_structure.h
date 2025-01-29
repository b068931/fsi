#ifndef THREAD_LOCAL_STRUCTURE_H
#define THREAD_LOCAL_STRUCTURE_H

#include "pch.h"
#include "scheduler.h"

struct thread_local_structure {
	module_mediator::arguments_string_type initializer{}; //contains data that will be passed to the main function in thread

	void* program_function_address{}; //will be used to store an address of the FSI function between calls to the "inner_create_thread" and "on_thread_creation"
	module_mediator::return_value priority; //store thread priority information between calls

	char* execution_thread_state{ new char[136] {} };   //state of the execution thread before switching to the program state
	scheduler::schedule_information currently_running_thread_information{};
};

#endif // !THREAD_LOCAL_STRUCTURE_H