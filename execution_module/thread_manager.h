#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "scheduler.h"
#include "module_interoperation.h"
#include "control_code_templates.h"

#include "../logger_module/logging.h"
#include "../module_mediator/local_crash_handle_setup.h"

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
        // These must be installed on a per-thread basis.
        module_mediator::crash_handling::install_crash_handlers();

        LOG_INFO(
            interoperation::get_module_part(), 
            std::format(
                "Executor {} is starting. System thread is {}.", 
                executor_id,
                GetCurrentThreadId()
            )
        );

        ULONG ulDesiredStackSize = 8192;
        BOOL bResult = SetThreadStackGuarantee(&ulDesiredStackSize);
        if (!bResult) {
            LOG_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Failed to set thread stack guarantee for executor {}. " \
                    "This may impede fatal error reporting.", 
                    executor_id
                )
            );
        }

        thread_local_structure* thread_structure = get_thread_local_structure();
        scheduler::schedule_information* currently_running_thread_information = 
            &thread_structure->currently_running_thread_information;

        while (true) {
            bool choose_result = this->scheduler.choose(currently_running_thread_information);
            if (!choose_result) {
                LOG_INFO(
                    interoperation::get_module_part(), 
                    std::format("Executor is shutting down.", executor_id)
                );

                break;
            }

            this->active_threads_counter.fetch_add(1, std::memory_order_relaxed); //all other modifications are synchronized with mutexes. this is one just needs atomicity
            CONTROL_CODE_TEMPLATE_LOAD_PROGRAM(
                thread_structure->execution_thread_state, 
                currently_running_thread_information->thread_state,
                currently_running_thread_information->state == scheduler::thread_states::startup
                    ? 1 : 0
            );

            std::size_t previous_active_threads_count = this->active_threads_counter.fetch_sub(1, std::memory_order_relaxed);
            if (currently_running_thread_information->put_back_structure) {
                this->scheduler.put_back(currently_running_thread_information->put_back_structure);
                for (const auto& deferred_callback : get_thread_local_structure()->deferred_callbacks) {
                    std::size_t module_index = interoperation::get_module_part()->find_module_index(deferred_callback->module_name);
                    if (module_index == module_mediator::module_part::module_not_found) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Module {} not found.", 
                                deferred_callback->module_name
                            )
                        );

                        continue;
                    }

                    std::size_t function_index = interoperation::get_module_part()->find_function_index(
                        module_index,
                        deferred_callback->function_name
                    );
                    if (function_index == module_mediator::module_part::function_not_found) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Function {} not found in module {}.", 
                                deferred_callback->function_name, 
                                deferred_callback->module_name
                            )
                        );

                        continue;
                    }

                    module_mediator::return_value result = interoperation::get_module_part()->call_module(
                        module_index,
                        function_index,
                        deferred_callback->arguments_string
                    );

                    if (result != module_mediator::module_success) {
                        LOG_WARNING(
                            interoperation::get_module_part(),
                            std::format(
                                "Deferred callback for module {} and function {} failed with error code {}.",
                                deferred_callback->module_name,
                                deferred_callback->function_name,
                                result
                            )
                        );
                    }
                }

                get_thread_local_structure()->deferred_callbacks.clear();
            }
            else {
                if (previous_active_threads_count == 1 && !this->scheduler.has_available_jobs()) {
                    LOG_INFO(interoperation::get_module_part(), "No available jobs found. Initiating shutdown."); //for this LOG_INFO the order is very important: it must be before the scheduler.initiate_shutdown() call
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
        assert(get_thread_local_structure()->currently_running_thread_information.thread_id == thread_id &&
            "Thread manager can only block the currently running thread.");

        this->scheduler.block(thread_id);
    }
    bool make_runnable(module_mediator::return_value thread_id) {
        return this->scheduler.make_runnable(thread_id);
    }
    void startup(std::uint16_t thread_count) {
        if(!this->scheduler.has_available_jobs()) {
            LOG_WARNING(interoperation::get_module_part(), "No available jobs found. Executors won't start.");
            return;
        }

        std::vector<std::thread> executors{};
        executors.reserve(thread_count);

        for (std::uint16_t counter = 0; counter < thread_count; ++counter) {
            executors.emplace_back(&thread_manager::executor_thread, this, counter);
        }

        for (std::thread& executor : executors) {
            executor.join();
        }
    }
};

#endif
