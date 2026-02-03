#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pch.h"
#include "clock_list.h"
#include "priority_list.h"

#include "../module_mediator/module_part.h"
#include "../startup_components/local_crash_handlers.h"

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
        // A fully unique identifier for a thread.
        module_mediator::return_value thread_id{};

        // A fully unique identifier for a thread group.
        // Notice that identifiers can't be the same for threads and thread groups.
        module_mediator::return_value thread_group_id{};

        // The priority of the program thread.
        module_mediator::return_value priority{
            std::numeric_limits<decltype(priority)>::min()
        };

        // Whether thread is running, runnable, etc. Look into an appropriate enum for more information.
        thread_states state{};
        
        // Values of processor registers that will be used to restore the program state.
        void* thread_state{};

        // Jump table address that is used to call functions in the program.
        const void* jump_table{};

        // Preferred stack size for the thread.
        std::uint64_t preferred_stack_size{};

        // Must be used with scheduler::put_back to allow a thread to be scheduled again.
        put_back_structure_type put_back_structure{}; 
    };

private:
    struct executable_thread {
        module_mediator::return_value id;

        // If locked, this means that the thread is already taken
        // to be honest, this feels kind of like a misuse of mutex, because
        // it is used to as data, not as a synchronization primitive.
        mutable std::mutex lock;
        thread_states state;
        
        void* state_buffer;
        const void* jump_table;

        executable_thread(
            module_mediator::return_value thread_id, 
            thread_states thread_initial_state, 
            void* thread_state_buffer, 
            const void* thread_jump_table
        )
            :id{ thread_id },
            state{ thread_initial_state },
            state_buffer{ thread_state_buffer },
            jump_table{ thread_jump_table }
        {}

        executable_thread(const executable_thread&) = delete;
        executable_thread& operator= (const executable_thread&) = delete;

        executable_thread(executable_thread&& thread) noexcept
            :id{ thread.id },
            state{ thread.state },
            state_buffer{ thread.state_buffer },
            jump_table{ thread.jump_table }
        {}

        executable_thread& operator= (executable_thread&& thread) noexcept {
            this->id = thread.id;
            this->state = thread.state;

            this->state_buffer = thread.state_buffer;
            this->jump_table = thread.jump_table;

            return *this;
        }

        ~executable_thread() noexcept = default;
    };

    struct thread_group {
        module_mediator::return_value id;
        std::uint64_t preferred_stack_size;

        mutable std::mutex lock;
        priority_list<executable_thread, module_mediator::return_value> threads;

        thread_group(
            module_mediator::return_value thread_group_id, 
            std::uint64_t thread_group_preferred_stack_size
        )
            :id{ thread_group_id },
            preferred_stack_size{ thread_group_preferred_stack_size }
        {}

        thread_group(const thread_group&) = delete;
        thread_group& operator= (const thread_group&) = delete;

        thread_group(thread_group&& thread_group) noexcept
            :id{ thread_group.id },
            preferred_stack_size{ thread_group.preferred_stack_size },
            threads{ std::move(thread_group.threads) }
        {}

        thread_group& operator= (thread_group&& thread_group) noexcept {
            this->id = thread_group.id;
            this->threads = std::move(thread_group.threads);
            this->preferred_stack_size = thread_group.preferred_stack_size;

            return *this;
        }

        ~thread_group() noexcept = default;
    };

    /*
    * NOTE(about unordered_map): "References and pointers to either key or data stored in the container 
    * are only invalidated by erasing that element, even when the corresponding iterator is invalidated."
    * https://en.cppreference.com/w/cpp/container/unordered_map
    */
    
    //synchronized with clock_list_mutex
    bool shutdown_sequence = false;

    std::unordered_map<module_mediator::return_value, clock_list<thread_group>::proxy> thread_groups_hash_table;
    std::mutex thread_groups_hash_table_mutex;

    std::unordered_map<module_mediator::return_value, priority_list<executable_thread, module_mediator::return_value>::proxy> threads_hash_table;
    std::recursive_mutex threads_hash_table_mutex;

    clock_list<thread_group> thread_groups;
    std::mutex clock_list_mutex;

    module_mediator::return_value runnable_threads_count{};
    std::condition_variable runnable_thread_notify;

    using thread_group_proxy = clock_list<thread_group>::proxy;
    using thread_proxy = priority_list<executable_thread, module_mediator::return_value>::proxy;

    thread_group_proxy& get_thread_group_using_hash_table(module_mediator::return_value id) {
        std::scoped_lock thread_group_hash_table_lock{ this->thread_groups_hash_table_mutex };
        return this->thread_groups_hash_table[id];
    }
    void remove_thread_group_from_hash_table(module_mediator::return_value thread_group_id) {
        std::scoped_lock thread_groups_hash_table_lock{ this->thread_groups_hash_table_mutex };

        auto found_thread_group = this->thread_groups_hash_table.find(thread_group_id);
        assert(
            found_thread_group != this->thread_groups_hash_table.end() &&
            "invalid thread group id when deleting a thread"
        );

        this->thread_groups_hash_table.erase(found_thread_group);
    }
    
    thread_proxy& get_thread_using_hash_table(module_mediator::return_value id) {
        std::scoped_lock threads_hash_table_lock{ this->threads_hash_table_mutex };
        return this->threads_hash_table[id];
    }
    void notify_runnable() {
        {
            std::scoped_lock clock_list_lock{ this->clock_list_mutex };
            ++this->runnable_threads_count;
        }

        this->runnable_thread_notify.notify_one();
    }

public:
    /*
    * Chooses the best thread for an executor. 
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
    * 2. repeat from step else. it is guaranteed that we will eventually stumble upon a thread group with 
    * a runnable thread because we reserved one thread by decrementing a runnable thread count.
    */
    bool choose(schedule_information* destination) {
        std::unique_lock clock_list_lock{ this->clock_list_mutex };

        while (true) {
            if (this->runnable_threads_count == 0) {
                //this variable is synchronized on a clock_list_mutex. look into "initiate_shutdown" function
                if (this->shutdown_sequence) {
                    return false;
                }

                this->runnable_thread_notify.wait(
                    clock_list_lock, 
                    [this] { return this->runnable_threads_count > 0 || this->shutdown_sequence; }
                );
            }
            else {
                --this->runnable_threads_count;
                while (true) {
                    thread_group* current_thread_group = this->thread_groups.get_current();
                    this->thread_groups.make_step(); //move to the next thread group

                    std::unique_lock thread_group_lock{ current_thread_group->lock };
                    clock_list_lock.unlock();

                    std::pair<executable_thread*, module_mediator::return_value> thread = current_thread_group->threads.find(
                        [destination, current_thread_group](const executable_thread& thread_object) {
                            if (thread_object.lock.try_lock()) { //check if thread is already taken
                                if (
                                    thread_object.state == thread_states::runnable || 
                                    thread_object.state == thread_states::startup
                                ) { //check if thread is runnable
                                    //lock will be released in put_back
                                    destination->preferred_stack_size = current_thread_group->preferred_stack_size;
                                    return true;
                                }
                                else {
                                    thread_object.lock.unlock();
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
                        destination->thread_state = thread.first->state_buffer;
                        destination->state = thread.first->state;
                        destination->put_back_structure = thread.first;

                        thread.first->state = thread_states::running;
                        return true;
                    }

                    clock_list_lock.lock(); 

                    //if search in this thread group was unsuccessful we try again. 
                    //see description above (SEARCH)
                }
            }
        }
    }

    /*
    * adding a new thread group. it can't be deleted until the first thread is added to it.
    * 1. lock on a clock_list mutex
    * 2. call clock_list.push_after() acquiring a proxy
    * 3. release clock_list mutex
    * 4. lock on a thread group hash table mutex
    * 5. add proxy to a thread group hash table
    * 6. release mutex
    */
    void add_thread_group(module_mediator::return_value id, std::uint64_t preferred_stack_size) {
        thread_group_proxy proxy;
        
        {
            std::scoped_lock clock_list_lock{ this->clock_list_mutex };
            proxy = this->thread_groups.push_after(id, preferred_stack_size);
        }

        std::scoped_lock thread_groups_hash_table_lock{ this->thread_groups_hash_table_mutex };
        this->thread_groups_hash_table[id] = std::move(proxy);
    }

    /*
    * adding a new thread to a thread group.
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
    void add_thread(
                    module_mediator::return_value thread_group_id, 
                    module_mediator::return_value thread_id, 
                    module_mediator::return_value priority,
                    void* thread_state,
                    const void* jump_table
    ) {
        {
            // Adding a thread to a thread group and adding it to the hash table must be done while we are holding a thread group lock.
            // This is to because it is possible that a new thread will start and complete before we finish adding it to the hash table.
            thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);

            std::scoped_lock thread_group_lock{ thread_group_proxy->lock };
            thread_proxy thread_proxy = thread_group_proxy->threads.push(
                priority,
                thread_id, thread_states::startup, thread_state, jump_table
            );

            std::scoped_lock thread_hash_table_lock{ this->threads_hash_table_mutex };
            this->threads_hash_table[thread_id] = std::move(thread_proxy);
        }

        this->notify_runnable();
    }

    /*
    * forgetting about the existence of a partly created thread_group.
    * (in other words, this function is called if execution module fails to add the first thread to a thread group)
    * 1. lock on a thread group hash table mutex
    * 2. find a thread group proxy
    * 3. release a thread group hash table mutex
    * 4. lock on a clock_list mutex
    * 5. lock on a thread_group mutex
    * 6. unlock a thread_group mutex
    * 7. remove thread_group and its entry in the thread group hash table
    */
    void forget_thread_group(module_mediator::return_value thread_group_id) {
        //see FORGET above
        thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);
        std::scoped_lock clock_list_lock{ this->clock_list_mutex };

        thread_group_proxy->lock.lock(); //here we make sure that no other threads are reading this structure
        assert(thread_group_proxy->threads.count() == 0 && "'forget' can be used only with empty thread_group");

        thread_group_proxy->lock.unlock();
        this->thread_groups.remove(std::move(thread_group_proxy));

        this->remove_thread_group_from_hash_table(thread_group_id);
    }

    /*
    * deleting a thread (or a thread AND a thread group). 
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
    * 5. release a thread group mutex - by doing these steps in this specific order, we eliminate the possibility of a deadlock, and in step 4 we make sure that no other threads are searching inside this thread group
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
    bool delete_thread(module_mediator::return_value thread_group_id, module_mediator::return_value thread_id) { //true if thread_group was deleted
        //see DELETING above
        thread_group_proxy& thread_group_proxy = this->get_thread_group_using_hash_table(thread_group_id);
        thread_proxy thread_proxy{};

        {
            std::scoped_lock threads_hash_table_lock{ this->threads_hash_table_mutex };
            
            auto found_thread = this->threads_hash_table.find(thread_id);
            assert(found_thread != this->threads_hash_table.end() && "invalid thread id when deleting a thread");

            thread_proxy = std::move(found_thread->second);
            this->threads_hash_table.erase(found_thread);
        }

        {
            std::unique_lock thread_group_lock{ thread_group_proxy->lock };

            thread_proxy->lock.unlock();
            thread_group_proxy->threads.remove(std::move(thread_proxy));

            if (thread_group_proxy->threads.count() == 0) {
                thread_group_lock.unlock(); //to avoid deadlock

                {
                    std::scoped_lock clock_list_lock{ this->clock_list_mutex };
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

    /* blocks a current thread.
    * 1. lock on a threads hash table mutex
    * 2. find a thread proxy
    * 3. release a threads hash table mutex
    * 4. lock on a thread mutex - we must NOT do this (check precondition 3)
    * 4. change state to "blocked"
    */
    void block(module_mediator::return_value thread_id) {
        thread_proxy& thread_proxy = this->get_thread_using_hash_table(thread_id);

        // Either way this should optimize out when compiling for release.
        // They both should not fire under normal circumstances.
        assert(thread_proxy.has_resource() && "unexpected no resource when blocking current thread");
        assert(thread_proxy->state == thread_states::running && "unexpected thread state when blocking current thread");

        thread_proxy->state = thread_states::blocked;
    }

    /*
    * make blocked thread runnable again.
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
    bool make_runnable(module_mediator::return_value thread_id) {
        // We do this to ensure that thread is actually blocked and not running or runnable.
        // Because if it is, then it is possible that it'll be removed just before we acquire the lock.
        // This, in turn, will lead to an undefined behavior.

        std::unique_lock threads_hash_table_lock{ this->threads_hash_table_mutex };
        thread_proxy& thread_proxy = this->get_thread_using_hash_table(thread_id);

        // This is even worse than terrible, because this means that we are trying to make a non-existent thread runnable.
        if (!thread_proxy.has_resource()) {
            [[maybe_unused]] std::size_t result = this->threads_hash_table.erase(thread_id);
            assert(result == 1 && "invalid thread id: something went really wrong");

            return false;
        }

        {
            if (thread_proxy->lock.try_lock()) {
                std::unique_lock thread_lock{thread_proxy->lock, std::adopt_lock};

                // We don't need this anymore, if we acquired the lock, then this means that the thread is either blocked or runnable.
                threads_hash_table_lock.unlock();
                if (thread_proxy->state == thread_states::blocked) {
                    thread_proxy->state = thread_states::runnable;
                    this->notify_runnable();

                    return true;
                }

                // This also terrible, because it means that the thread is runnable or startup.
                return false;
            }
            else {
                // This is terrible, because it means that the thread is either running.
                return false;
            }
        }
    }

    /*
    * return a thread to scheduler after receiving a switch command. 
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

    bool has_available_jobs() {
        std::scoped_lock thread_groups_hash_table_lock{ this->thread_groups_hash_table_mutex };
        return !this->thread_groups_hash_table.empty();
    }

    void initiate_shutdown() { 
        //locking on this mutex is required in order to ensure that all executors are either already waiting on a condition variable or are yet to enter the "choose" function.
        std::scoped_lock clock_list_lock{ this->clock_list_mutex };
        if (!this->threads_hash_table.empty() || !this->thread_groups_hash_table.empty()) {
            // Make a panic shutdown if hash tables got out of sync
            ENVIRONMENT_REQUEST_TERMINATION();
        }

        this->shutdown_sequence = true;
        this->runnable_thread_notify.notify_all();
    }
};

#endif // !SCHEDULER_H
