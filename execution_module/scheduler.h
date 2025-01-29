#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pch.h"
#include "clock_list.h"
#include "priority_list.h"

/*
* GENERAL PRECONDITIONS. if these preconditions are met, following algorithms should work properly
* 1. threads from thread group A must NOT manipulate threads from thread group B in any way, except
* from the cases when changing the state from "blocked" to "runnable" (after this any interaction is prohibited)
* 2. a thread can be deleted ONLY by an executor that acquired the schedule_information_structure after SEARCH
* (this rule ensures that a newly created empty thread group won't be deleted before the first thread gets added to it)
* 3. executor can change the state of its current thread to blocked and then switch to another thread
*/

/*
* (scheduler::choose) SEARCH. chooses the best thread for an executor. 
* 1. lock on a clock_list mutex
* 2. check runnable thread count
* 
* if == 0 => block on condition variable
* else 
* 1. decrement runnable thread count 
* 2. choose thread group
* 3. lock on a thread group mutex
* 4. release clock_list mutex
* 5. attempt to find a thread with the highest priority in a group
* 
* if successful:
* 1. acquire thread mutex
* 2. release thread group mutex
* 3. return
* 
* if not:
* 1. lock on a clock_list mutex
* 2. repeat from step else.2 - it is guaranteed that we will eventually stumble upon a thread group with 
* a runnable thread because we reserved one thread by decrementing a runnable thread count.
*/

/*
* (scheduler::add_thread_group) ADDING a new thread group.
* 1. lock on a clock_list mutex
* 2. call clock_list.push_after() acquiring a proxy
* 3. release clock_list mutex
* 4. lock on a thread group hash table mutex
* 5. add proxy to a thread group hash table
* 6. release mutex
*/

/*
* (scheduler::add_thread) ADDING a new thread to a thread group.
* 1. lock on a thread group hash table mutex
* 2. find a thread group proxy
* 3. release a thread group hash table mutex
* 4. lock on a thread group mutex using this proxy
* 5. call priority_list.push acquiring a proxy
* 6. release a thread group mutex
* 7. lock on a threads hash table mutex
* 8. add proxy to a threads hash table
* 9. release a threads hash table mutex
* 10. lock on a clock_list mutex
* 11. increment runnable threads count
* 12. release a clock_list mutex
* 13. notify condition variable 
*/

/*
* (scheduler::forget_thread_group) FORGETING about the existance of a partly created thread_group. (in other words, this function is called if excm fails to add the first thread to a thread group)
* 1. lock on a thread group hash table mutex
* 2. find a thread group proxy
* 3. release a thread group hash table mutex
* 4. lock on a clock_list mutex
* 5. lock on a thread_group mutex
* 6. unlock a thread_group mutex
* 7. remove thread_group and its entry in the thread group hash table
*/

/*
* (scheduler::delete_thread) DELETING a thread (or a thread AND a thread group). 
* NOTICE: thread group won't be deleted if it has at least 1 thread in it
* 1. lock on a thread group hash table mutex
* 2. find a thread group proxy
* 3. release a thread group hash table mutex
* 4. lock on a threads hash table mutex
* 5. move a thread proxy to a new object, and delete an entry in threads hash table mutex
* 6. release a threads hash table mutex
* 7. lock on a thread group mutex
* 8. lock on a thread mutex - DO NOT DO THIS, we already acquired this mutex before switching to a program state (see preconditions)
* 8. release a thread mutex - because it is an UB to destroy a mutex while it is still being held
* 9. priority_list.remove(thread proxy) 
* 10. thread group.thread count == 0
* 
* if true:
* 1. release a thread group mutex
* 2. lock on a clock_list mutex
* 4. lock on a thread group mutex
* 5. release a thread group mutex - by doing these steps in this specific order, we eliminate the possiblity of a deadlock, and in step 4 we make sure that no other threads are searching inside this thread group
* 6. clock_list.remove(thread group proxy)
* 7. release clock_list mutex
* 8. lock on a thread group hash table mutex
* 9. delete entry in hash table
* 10. release thread group hash table mutex - at this point, we can be sure that no other threads are using a proxy, 
* because no other threads were using it before (check step 4), and we deleted a thread group in step 6
* 
* if false:
* 1. release thread group mutex
*/

/* (scheduler::block) BLOCK.
* 1. lock on a threads hash table mutex
* 2. find a thread proxy
* 3. release a threads hash table mutex
* 4. lock on a thread mutex - we must NOT do this (check precondition 3)
* 4. change state to "blocked"
*/

/*
* (scheduler::make_runnable) MAKE RUNNABLE. 
* 1. lock on a threads hash table mutex
* 2. find a thread proxy
* 3. release a threads hash table mutex
* 4. lock on a thread mutex
* 5. change state
* 6. release a thread mutex
* 7. lock on a clock_list mutex
* 8. increment runnable thread count
* 9. release a clock_list mutex
* 10. notify condition variable
*/

/*
* (scheduler::put_back) PUT BACK. 
* 1. thread state == "running" || == "startup"
* 
* if true:
* 1. change state to "runnable"
* 2. release a thread mutex
* 3. lock on a clock_list mutex
* 4. increment runnable thread count
* 5. release a clock_list mutex
* 6. notify condition variable
* 
* else:
* 1. release a thread mutex
*/

class scheduler {
public:
	using put_back_structure_type = void*;

	enum class thread_states {
		running,
		runnable,
		blocked,
		startup
	};

	struct schedule_information {
		module_mediator::return_value thread_id;
		module_mediator::return_value thread_group_id;

		module_mediator::return_value priority;
		thread_states state;
		
		void* thread_state;
		const void* jump_table;
		put_back_structure_type put_back_structure; //must be used with scheduler::put_back
	};

private:
	struct executable_thread {
		module_mediator::return_value id;

		//if locked, this means that the thread is already taken
		std::mutex lock;
		thread_states state;
		
		void* thread_state;
		const void* jump_table;

		executable_thread(module_mediator::return_value id, thread_states state, void* thread_state, const void* jump_table)
			:state{ state },
			thread_state{ thread_state },
			jump_table{ jump_table },
			id{ id }
		{}

		executable_thread(executable_thread&& thread) noexcept
			:lock{},
			state{ thread.state },
			thread_state{ thread.thread_state },
			jump_table{ thread.jump_table },
			id{ thread.id }
		{}
		void operator= (executable_thread&& thread) noexcept {
			this->id = thread.id;
			this->state = thread.state;

			this->thread_state = thread.thread_state;
			this->jump_table = thread.jump_table;
		}
	};

	struct thread_group {
		module_mediator::return_value id;

		std::mutex lock;
		priority_list<executable_thread, module_mediator::return_value> threads;

		thread_group(module_mediator::return_value id)
			:id{id}
		{}

		thread_group(thread_group&& thread_group) noexcept
			:id{ thread_group.id },
			threads{ std::move(thread_group.threads) }
		{}
		void operator= (thread_group&& thread_group) noexcept {
			this->id = thread_group.id;
			this->threads = std::move(thread_group.threads);
		}
	};

	/*
	* NOTE(about unordered_map): "References and pointers to either key or data stored in the container 
	* are only invalidated by erasing that element, even when the corresponding iterator is invalidated."
	* https://en.cppreference.com/w/cpp/container/unordered_map
	*/

	std::unordered_map<module_mediator::return_value, clock_list<thread_group>::proxy> thread_groups_hash_table;
	std::mutex thread_groups_hash_table_mutex;

	std::unordered_map<module_mediator::return_value, priority_list<executable_thread, module_mediator::return_value>::proxy> threads_hash_table;
	std::mutex threads_hash_table_mutex;

	module_mediator::return_value runnable_threads_count{};
	clock_list<thread_group> thread_groups;
	std::mutex clock_list_mutex;

	std::condition_variable runnable_thread_notify;

	using thread_group_proxy = clock_list<thread_group>::proxy;
	using thread_proxy = priority_list<executable_thread, module_mediator::return_value>::proxy;

	thread_group_proxy& get_thread_group_using_hash_table(module_mediator::return_value id) {
		std::lock_guard<std::mutex> thread_group_hash_table_lock{ this->thread_groups_hash_table_mutex };
		return this->thread_groups_hash_table[id];
	}
	void remove_thread_group_from_hash_table(module_mediator::return_value thread_group_id) {
		std::lock_guard<std::mutex> thread_groups_hash_table_lock{ this->thread_groups_hash_table_mutex };

		auto found_thread_group = this->thread_groups_hash_table.find(thread_group_id);
		assert(
			found_thread_group != this->thread_groups_hash_table.end() &&
			"invalid thread group id when deleting a thread"
		);

		this->thread_groups_hash_table.erase(found_thread_group);
	}
	
	thread_proxy& get_thread_using_hash_table(module_mediator::return_value id) {
		std::lock_guard<std::mutex> threads_hash_table_lock{ this->threads_hash_table_mutex };
		return this->threads_hash_table[id];
	}
	void notify_runnable() {
		{
			std::lock_guard<std::mutex> clock_list_lock{ this->clock_list_mutex };
			++this->runnable_threads_count;
		}

		this->runnable_thread_notify.notify_one();
	}

public:
	void choose(schedule_information* destination) {
		std::unique_lock<std::mutex> clock_list_lock{ this->clock_list_mutex };

		while (true) {
			if (this->runnable_threads_count == 0) {
				this->runnable_thread_notify.wait(
					clock_list_lock, 
					[this] { return this->runnable_threads_count > 0; }
				);
			}
			else {
				--this->runnable_threads_count;

				while (true) {
					thread_group* current_thread_group = this->thread_groups.get_current();
					this->thread_groups.make_step(); //move to the next thread group

					std::unique_lock<std::mutex> thread_group_lock{ current_thread_group->lock };
					clock_list_lock.unlock();

					std::pair<executable_thread*, module_mediator::return_value> thread = current_thread_group->threads.find(
						[](executable_thread& thread_obj) {
							if (thread_obj.lock.try_lock()) { //check if thread is already taken
								if (
									(thread_obj.state == thread_states::runnable) || 
									(thread_obj.state == thread_states::startup)
								) { //check if thread is runnable
									//lock will be released in put_back
									return true;
								}
								else {
									thread_obj.lock.unlock();
								}
							}

							return false;
						}
					);

					thread_group_lock.unlock();					
					if (thread.first != nullptr) { //we have already acquired a thread->lock by using a try_lock function
						/*
						* notice that we access current_thread_group after releasing the thread_group_lock.
						* this should work fine because current_thread_group->id is a separate memory location,
						* and no other threads write to it. also, in this case current_thread_group
						* always points to a valid location in memory because pointers are invalidated only if
						* specific object is deleted, and a thread group can not be deleted if it has at least one 
						* thread. if we are in this branch,  we are holding a lock to a thread inside 
						* this thread group, making it impossible to delete this thread and, subsequently, its 
						* thread group.
						*/

						destination->priority = thread.second;
						destination->thread_id = thread.first->id;
						destination->thread_group_id = current_thread_group->id;
						destination->jump_table = thread.first->jump_table;
						destination->thread_state = thread.first->thread_state;
						destination->state = thread.first->state;
						destination->put_back_structure = thread.first;

						thread.first->state = thread_states::running;
						return;
					}

					clock_list_lock.lock(); 

					//if search in this thread group was unsuccessful we try again. 
					//see description above (SEARCH)
				}
			}
		}
	}
	void add_thread_group(module_mediator::return_value id) {
		thread_group_proxy proxy{};
		
		{
			std::lock_guard<std::mutex> clock_list_lock{ this->clock_list_mutex };
			proxy = this->thread_groups.push_after(id);
		}

		std::lock_guard<std::mutex> thread_groups_hash_table_lock{ this->thread_groups_hash_table_mutex };
		this->thread_groups_hash_table[id] = std::move(proxy);
	}
	void add_thread(
					module_mediator::return_value thread_group_id, 
					module_mediator::return_value thread_id, 
					module_mediator::return_value priority,
					void* thread_state,
					const void* jump_table
	) {
		thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);

		thread_proxy thread_proxy{};
		{
			std::lock_guard<std::mutex> thread_group_lock{ thread_group_proxy->lock };
			thread_proxy = thread_group_proxy->threads.push(
				priority, 
				thread_id, thread_states::startup, thread_state, jump_table
			);
		}

		{
			std::lock_guard<std::mutex> thread_hash_table_lock{ this->threads_hash_table_mutex };
			this->threads_hash_table[thread_id] = std::move(thread_proxy);
		}

		this->notify_runnable();
	}
	void forget_thread_group(module_mediator::return_value thread_group_id) {
		//see FORGET above
		thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);
		std::lock_guard<std::mutex> clock_list_lock{ this->clock_list_mutex };

		thread_group_proxy->lock.lock(); //here we make sure that no other threads are reading this structure
		assert(thread_group_proxy->threads.count() == 0 && "'forget' can be used only with empty thread_group");

		thread_group_proxy->lock.unlock();
		this->thread_groups.remove(std::move(thread_group_proxy));

		this->remove_thread_group_from_hash_table(thread_group_id);
	}
	bool delete_thread(module_mediator::return_value thread_group_id, module_mediator::return_value thread_id) { //true if thread_group was deleted
		//see DELETING above
		thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);
		
		thread_proxy thread_proxy{};
		{
			std::lock_guard<std::mutex> threads_hash_table_lock{ this->threads_hash_table_mutex };
			
			auto found_thread = this->threads_hash_table.find(thread_id);
			assert(found_thread != this->threads_hash_table.end() && "invalid thread id when deleting a thread");

			thread_proxy = std::move(found_thread->second);
			this->threads_hash_table.erase(found_thread);
		}

		{
			std::unique_lock<std::mutex> thread_group_lock{ thread_group_proxy->lock };

			thread_proxy->lock.unlock();
			thread_group_proxy->threads.remove(std::move(thread_proxy));

			if (thread_group_proxy->threads.count() == 0) {
				thread_group_lock.unlock(); //to avoid deadlock

				{
					std::lock_guard<std::mutex> clock_list_lock{ this->clock_list_mutex };
					thread_group_lock.lock();

					//we lock and instantly unlock this mutex to ensure that no other threads are reading this structure
					thread_group_lock.unlock();
					this->thread_groups.remove(std::move(thread_group_proxy));
				}

				this->remove_thread_group_from_hash_table(thread_group_id);
				return true;
			}
		} //if this thread group has other threads, lock will be released automatically here

		return false;
	}
	void block(module_mediator::return_value thread_id) {
		//see BLOCK above

		thread_proxy& thread_proxy = this->get_thread_using_hash_table(thread_id);
		thread_proxy->state = thread_states::blocked;
	}
	void make_runnable(module_mediator::return_value thread_id) {
		thread_proxy& thread_proxy = this->get_thread_using_hash_table(thread_id);

		{
			std::lock_guard<std::mutex> thread_lock{ thread_proxy->lock };
			thread_proxy->state = thread_states::runnable;
		}

		this->notify_runnable();
	}
	void put_back(put_back_structure_type put_back_structure) {
		executable_thread* thread = static_cast<executable_thread*>(put_back_structure);
		if (thread->state == thread_states::running) {
			thread->state = thread_states::runnable;
			
			//thread->lock was acquired by another function (specifically "scheduler::choose")
			thread->lock.unlock();
			this->notify_runnable();
		}
		else {
			thread->lock.unlock();
		}
	}
};

#endif // !SCHEDULER_H
