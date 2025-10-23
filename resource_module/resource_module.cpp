#include "pch.h"
#include "resource_module.h"

#include "program_container.h"
#include "thread_structure.h"
#include "id_generator.h"
#include "module_interoperation.h"

#include "../logger_module/logging.h"
#include "../module_mediator/fsi_types.h"

#ifndef _MSC_VER
    #define _Acquires_lock_(a)
    #define _Releases_lock_(a)
#endif

namespace {
    std::map<id_generator::id_type, program_container> containers;
    std::recursive_mutex containers_mutex;

    std::map<id_generator::id_type, thread_structure> thread_structures;
    std::recursive_mutex thread_structures_mutex;

    template<typename T>
    _Acquires_lock_(return.second) auto get_iterator(T& object, std::recursive_mutex& mutex, id_generator::id_type id) {
        /*
        * here we at first lock mutex associated with a map,
        * look for an object, lock mutex associated with an object,
        * release mutex associated with a map, modify our object,
        * release mutex associated with an object.
        *
        * std::map does not invalidate iterators,
        * so even if some thread deletes another object while we modify a current object, nothing will happen.
        *
        * a running object can not be deleted, but even if someone tries to do so,
        * nothing will happen until the lock to an object is released or object.end() will be returned
        */

        std::lock_guard lock{ mutex };

        auto iterator = object.find(id);
        if (iterator != object.end()) {
            std::unique_lock object_lock{ *iterator->second.lock };
            return std::pair{ iterator, std::move(object_lock) };
        }

        LOG_WARNING(
            interoperation::get_module_part(),
            std::format("Object with ID {} does not exist.", id)
        );

        return
            std::pair{
                object.end(), //won't be actually accessed anywhere in the program because map can be modified after this function
                std::unique_lock<
                    std::remove_reference_t<
                        decltype(*iterator->second.lock)
                    >
                >{}
        };
    }

    template<typename T>
    void add_destroy_callback_generic(
        std::recursive_mutex& mutex, 
        T& object, 
        id_generator::id_type id, 
        module_mediator::callback_bundle* bundle
    ) {
        auto iterator_lock = get_iterator(object, mutex, id);
        if (iterator_lock.second) {
            iterator_lock.first->second.destroy_callbacks.push_back(bundle);
        }
        else {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Concurrency error: failed to add destroy callback for an object with id {}. It no longer exists.", 
                    id
                )
            );
        }
    }

    template<typename T>
    std::uintptr_t allocate_memory_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id, std::uint64_t size) {
        auto iterator_lock = get_iterator(object, mutex, id);

        /*
        * in theory, this operation should always be successful
        * because program container can not be destroyed while it has at least one running thread,
        * and the memory belonging to a program can be allocated only by its own threads.
        * so "bool(iterator_lock.second)" works mostly as a precaution.
        */

        if (iterator_lock.second) { //check if we acquired mutex for an object
            [[maybe_unused]] auto [result, is_new] = iterator_lock.first->second.allocated_memory.insert(
                static_cast<void*>(new(std::nothrow) char[size] {})
            );

            assert(is_new && "allocated memory already exists for this object");
            return reinterpret_cast<std::uintptr_t>(*result);
        }

        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(), 
            std::format(
                "Concurrency error: failed to allocate memory for an object with id {}. It no longer exists.",
                id
            )
        );

        return reinterpret_cast<std::uintptr_t>(nullptr);
        _Releases_lock_(iterator_lock->second);
    }

    template<typename T>
    void deallocate_memory_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id, void* address) {
        //see allocate_memory_generic
        auto iterator_lock = get_iterator(object, mutex, id);
        if (iterator_lock.second) {
            auto& allocated_memory = iterator_lock.first->second.allocated_memory;
            auto found_address = allocated_memory.find(address);

            //if address does not belong to this structure we do nothing
            if (found_address != allocated_memory.end()) {
                delete[] static_cast<char*>(*found_address);
                allocated_memory.erase(found_address);
            }
            else {
                LOG_PROGRAM_WARNING(
                    interoperation::get_module_part(), 
                    std::format(
                        "Deallocated memory at {} does not belong to the object with id {}.",
                        address,
                        id
                    )
                );
            }
        }
        else {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Concurrency error: failed to deallocate memory for an object with id {}. It no longer exists.",
                    id
                )
            );
        }
    }

    template<typename T>
    std::conditional_t<
        std::is_same_v<T, std::map<id_generator::id_type, thread_structure>>, //for thread_structure this function also returns id of the associated program_container
        std::pair<module_mediator::return_value, id_generator::id_type>,
        module_mediator::return_value
    >
        deallocate_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id) {
        constexpr bool thread_structure_switch = std::is_same_v<T, std::map<id_generator::id_type, thread_structure>>;

        [[maybe_unused]] id_generator::id_type program_container_id = 0;
        id_generator::id_type free_id = 0;
        {
            std::unique_lock lock{ mutex };
            auto iterator_lock = get_iterator(object, mutex, id);
            if constexpr (!thread_structure_switch) {
                if (iterator_lock.first->second.threads_count != 0) {
                    LOG_PROGRAM_WARNING(
                        interoperation::get_module_part(),
                        std::format(
                            "Concurrency error: failed to destroy thread group with id {}. It still has running threads.",
                            id
                        )
                    );

                    return module_mediator::module_failure;
                }
            }

            /*
            * in theory, if several threads end simultaneously, it can lead to multiple calls to this function,
            * so we need to make sure that nothing bad happens
            */

            if (iterator_lock.second) {
                auto container{ std::move(iterator_lock.first->second) }; //destructor of this class will free resources

                /*
                * "The behavior of a program is undefined if a recursive_mutex is destroyed while still owned by some thread."
                * https://en.cppreference.com/w/cpp/thread/recursive_mutex
                */

                iterator_lock.second.unlock();
                object.erase(iterator_lock.first);

                lock.unlock(); //at this point the object is deleted and if several other threads were waiting on the mutex while we were deleting the object they will find nothing
                free_id = id;

                if constexpr (thread_structure_switch) {
                    program_container_id = container.program_container;
                }
            }
        }

        if (free_id != 0) {
            id_generator::free_id(free_id);

            if constexpr (thread_structure_switch) {
                return std::pair{ module_mediator::module_success, program_container_id };
            }
            else {
                return module_mediator::module_success;
            }
        }

        if constexpr (thread_structure_switch) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(), 
                std::format(
                    "Concurrency error: failed to destroy thread with id {}.",
                    id
                )
            );

            return std::pair{ module_mediator::module_failure, 0 };
        }
        else {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(), 
                std::format(
                    "Concurrency error: failed to destroy thread group with id {}.",
                    id
                )
            );

            return module_mediator::module_failure;
        }
    }

    template<typename T>
    module_mediator::return_value verify_memory_generic(
        std::recursive_mutex& mutex,
        T& object,
        id_generator::id_type object_id, 
        module_mediator::memory address
    ) {
        if (address == nullptr) {
            return module_mediator::module_failure;
        }

        auto [iterator, lock] =
            get_iterator(object, mutex, object_id);

        if (lock) {
            if (iterator->second.allocated_memory.contains(address)) {
                return module_mediator::module_success;
            }

            return module_mediator::module_failure;
        }

        LOG_PROGRAM_WARNING(
            interoperation::get_module_part(),
            std::format("Object with id {} does not exist, cannot verify memory at {}.", 
                object_id, 
                reinterpret_cast<std::uintptr_t>(address)
            )
        );

        return module_mediator::module_failure;
    }

    void insert_new_container(id_generator::id_type id, program_context* context) {
        program_container new_container{};
        new_container.context = context;

        std::lock_guard lock{ containers_mutex };
        containers[id] = std::move(new_container);
    }
    module_mediator::return_value notify_execution_module_new_container(
        id_generator::id_type id,
        void* main_function,
        std::uint64_t preferred_stack_size
    ) {
        return module_mediator::fast_call<
            module_mediator::return_value,
            module_mediator::memory,
            module_mediator::eight_bytes
        >(
            interoperation::get_module_part(),
            interoperation::index_getter::execution_module(),
            interoperation::index_getter::execution_module_on_container_creation(),
            id,
            main_function,
            preferred_stack_size
        );
    }
}

module_mediator::return_value add_container_on_destroy(module_mediator::arguments_string_type bundle) {
    auto [container_id, callback_bundle] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::memory>(bundle);

    add_destroy_callback_generic(
        containers_mutex,
        containers,
        container_id,
        static_cast<module_mediator::callback_bundle*>(callback_bundle)
    );

    return module_mediator::module_success;
}
module_mediator::return_value add_thread_on_destroy(module_mediator::arguments_string_type bundle) {
    auto [thread_id, callback_bundle] = 
        module_mediator::arguments_string_builder::unpack<module_mediator::return_value, module_mediator::memory>(bundle);

    add_destroy_callback_generic(
        thread_structures_mutex,
        thread_structures,
        thread_id,
        static_cast<module_mediator::callback_bundle*>(callback_bundle)
    );

    return module_mediator::module_success;
}

module_mediator::return_value duplicate_container(module_mediator::arguments_string_type bundle) {
    auto [container_id, main_function] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, module_mediator::memory>(bundle);

    auto [iterator, lock] =
        get_iterator(containers, containers_mutex, container_id);

    if (lock) {
        id_generator::id_type new_container_id = id_generator::get_id();
        insert_new_container(
            new_container_id,
            program_context::duplicate(iterator->second.context)
        );

        lock.unlock(); // unlock before doing logging
        LOG_PROGRAM_INFO(interoperation::get_module_part(), "Duplicating the program context.");

        return notify_execution_module_new_container(
            new_container_id,
            main_function,
            iterator->second.context->preferred_stack_size
        );
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Concurrency error: failed to duplicate container with id {}.", 
            container_id
        )
    );

    return module_mediator::module_failure;
}
module_mediator::return_value get_preferred_stack_size(module_mediator::arguments_string_type bundle) {
    constexpr std::uint64_t fallback_stack_size = 1024;
    auto [container_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [iterator, lock] =
        get_iterator(containers, containers_mutex, container_id);

    if (lock) {
        return iterator->second.context->preferred_stack_size;
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Thread group with id {} no longer exists. Using a fallback stack size of {}.", 
            container_id, 
            fallback_stack_size
        )
    );

    return fallback_stack_size;
}

module_mediator::return_value create_new_program_container(module_mediator::arguments_string_type bundle) {
    id_generator::id_type id = id_generator::get_id();
    auto [preferred_stack_size, main_function_index, 
          compiled_functions, compiled_functions_count, 
          exposed_functions, exposed_functions_count,
          jump_table, jump_table_size,
          program_strings, program_strings_count] = module_mediator::arguments_string_builder::unpack<
        std::uint64_t, std::uint32_t, 
        module_mediator::memory, std::uint32_t, 
        module_mediator::memory, std::uint32_t, 
        module_mediator::memory, std::uint64_t, 
        module_mediator::memory, std::uint64_t
    >(bundle);
    
    /*
    * a newly created object can not be deleted or modified if execution module does not know about it,
    * so we don't need to lock on its mutex
    */

    insert_new_container(
        id,
        program_context::create(
            preferred_stack_size,
            static_cast<void**>(compiled_functions), compiled_functions_count, 
            static_cast<void**>(exposed_functions), exposed_functions_count, 
            jump_table, jump_table_size,
            static_cast<void**>(program_strings), program_strings_count
        )
    );

    return notify_execution_module_new_container(
        id, 
        static_cast<void**>(compiled_functions)[main_function_index], 
        preferred_stack_size
    );
}
module_mediator::return_value create_new_thread(module_mediator::arguments_string_type bundle) {
    auto [container_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    std::uint64_t preferred_stack_size;
    id_generator::id_type id = id_generator::get_id();

    /*
    * despite the fact that get_iterator can fail,
    * it should never happen in this case because a new thread can be created only by an already existing thread,
    * and creating thread can not continue its execution until a new thread is created
    */

    { // make sure the lock is gone before calling to another module
        auto [iterator, lock] = 
            get_iterator(containers, containers_mutex, container_id);

        if (lock) {
            assert(iterator->first == container_id && "unexpected container id");

            {
                std::lock_guard threads_lock{ thread_structures_mutex };
                thread_structures[id] = thread_structure{ iterator->first };
            }

            preferred_stack_size = iterator->second.context->preferred_stack_size;
            ++iterator->second.threads_count;
        }
        else {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Concurrency error: failed to create thread in container with id {}. It no longer exists.",
                    container_id
                )
            );

            return module_mediator::module_failure;
        }
    }

    return module_mediator::fast_call<
        module_mediator::return_value,
        module_mediator::return_value,
        module_mediator::eight_bytes
    >(
        interoperation::get_module_part(),
        interoperation::index_getter::execution_module(),
        interoperation::index_getter::execution_module_on_thread_creation(),
        container_id,
        id,
        preferred_stack_size
    );
}

module_mediator::return_value allocate_program_memory(module_mediator::arguments_string_type bundle) {
    auto [container_id, memory_size] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, std::uint64_t>(bundle);

    return allocate_memory_generic(containers_mutex, containers, container_id, memory_size);
}
module_mediator::return_value allocate_thread_memory(module_mediator::arguments_string_type bundle) {
    auto [thread_id, memory_size] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, std::uint64_t>(bundle);

    return allocate_memory_generic(thread_structures_mutex, thread_structures, thread_id, memory_size);
}

module_mediator::return_value deallocate_program_memory(module_mediator::arguments_string_type bundle) {
    auto [container_id, memory_address] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, module_mediator::memory>(bundle);

    deallocate_memory_generic(containers_mutex, containers, container_id, memory_address);
    return module_mediator::module_success;
}
module_mediator::return_value deallocate_thread_memory(module_mediator::arguments_string_type bundle) {
    auto [thread_id, memory_address] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, module_mediator::memory>(bundle);

    deallocate_memory_generic(thread_structures_mutex, thread_structures, thread_id, memory_address);
    return module_mediator::module_success;
}

module_mediator::return_value deallocate_program_container(module_mediator::arguments_string_type bundle) {
    auto [container_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    return deallocate_generic(containers_mutex, containers, container_id);
}
module_mediator::return_value deallocate_thread(module_mediator::arguments_string_type bundle) {
    auto [thread_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [return_code, container_id] = 
        deallocate_generic(thread_structures_mutex, thread_structures, thread_id);

    if (return_code != module_mediator::module_failure) {
        auto [iterator, lock] = 
            get_iterator(containers, containers_mutex, container_id);

        if (lock) {
            --iterator->second.threads_count;
        }
        else {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Concurrency error: failed to decrease running threads count for a program container with id {}. It no longer exists.",
                    container_id
                )
            );

            return module_mediator::module_failure;
        }

        return container_id;
    }

    // deallocate generic will generate a warning
    return module_mediator::module_failure;
}

module_mediator::return_value get_running_threads_count(module_mediator::arguments_string_type bundle) {
    auto [container_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [iterator, lock] = 
        get_iterator(containers, containers_mutex, container_id);

    if (lock) {
        return iterator->second.threads_count;
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Concurrency error: failed to get running threads count for a program container with id {}. It no longer exists.",
            container_id
        )
    );

    return module_mediator::module_failure; 
}
module_mediator::return_value get_program_container_id(module_mediator::arguments_string_type bundle) {
    auto [thread_id] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [iterator, lock] =
        get_iterator(thread_structures, thread_structures_mutex, thread_id);

    if (lock) {
        return iterator->second.program_container;
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Concurrency error: failed to get program container id for a thread with id {}. It no longer exists.",
            thread_id
        )
    );

    /*
    * ::max() means that the requested program container does not exist.
    * we don't return 0 because an index to this program container might have already been reused
    */

    return module_mediator::module_failure;
}

module_mediator::return_value get_jump_table(module_mediator::arguments_string_type bundle) {
    auto [container_id] = 
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [iterator, lock] = 
        get_iterator(containers, containers_mutex, container_id);

    if (lock) {
        return reinterpret_cast<std::uintptr_t>(iterator->second.context->jump_table);
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Concurrency error: failed to get jump table for a program container with id {}. It no longer exists.",
            container_id
        )
    );
    
    return reinterpret_cast<std::uintptr_t>(nullptr);
}
module_mediator::return_value get_jump_table_size(module_mediator::arguments_string_type bundle) {
    auto [container_id] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);

    auto [iterator, lock] =
        get_iterator(containers, containers_mutex, container_id);

    if (lock) {
        return iterator->second.context->jump_table_size;
    }

    LOG_PROGRAM_WARNING(
        interoperation::get_module_part(),
        std::format(
            "Concurrency error: failed to get jump table size for a program container with id {}. It no longer exists.",
            container_id
        )
    );

    return module_mediator::module_failure;
}

module_mediator::return_value verify_thread_memory(module_mediator::arguments_string_type bundle) {
    auto [thread_id, memory_address] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, module_mediator::memory>(bundle);

    return verify_memory_generic(
        thread_structures_mutex,
        thread_structures,
        thread_id,
        memory_address
    );
}
module_mediator::return_value verify_program_memory(module_mediator::arguments_string_type bundle) {
    auto [container_id, memory_address] =
        module_mediator::arguments_string_builder::unpack<id_generator::id_type, module_mediator::memory>(bundle);

    return verify_memory_generic(
        containers_mutex,
        containers,
        container_id,
        memory_address
    );
}

program_container::~program_container() noexcept {
    // execution module will make sure that there are no active threads before destroying a program container
    // this should happen only on program close (e.g. console got closed before the program is done)
    // at this point logger module may have been unloaded already, so we can only use std::cerr here
    if (this->threads_count == 0) {
        if (this->context && this->context->decrease_references_count() == 0) {
            delete this->context;
        }
    }
    else {
        std::osyncstream standard_error{ std::cerr };
        standard_error << std::format(
            "*** Warning: abandoning program container with {} active thread(s).\n",
            this->threads_count
        );
    }
}

program_context::~program_context() noexcept {
    assert(this->references_count == 0 && "destroying program context that has active references");
    module_mediator::fast_call<
        module_mediator::memory, module_mediator::four_bytes,
        module_mediator::memory, module_mediator::four_bytes,
        module_mediator::memory,
        module_mediator::memory, module_mediator::eight_bytes
    >(
        interoperation::get_module_part(),
        interoperation::index_getter::program_loader(),
        interoperation::index_getter::program_loader_free_program(),
        static_cast<void*>(this->code),
        this->functions_count,
        static_cast<void*>(this->exposed_functions),
        this->exposed_functions_count,
        this->jump_table,
        static_cast<void*>(this->program_strings),
        this->program_strings_size
    );
}