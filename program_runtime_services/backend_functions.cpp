#include "pch.h"
#include "backend_functions.h"

#include "../logger_module/logging.h"

namespace backend {
    std::pair<void*, std::uint64_t> decay_pointer(module_mediator::memory value) {
        if (value == nullptr)
            return { nullptr, 0 };

        if (interoperation::verify_thread_memory(interoperation::get_current_thread_id(), value) == module_mediator::module_failure) {
            LOG_PROGRAM_WARNING(
                interoperation::get_module_part(),
                std::format(
                    "Thread memory at {} does not belong to {}",
                    reinterpret_cast<std::uintptr_t>(value),
                    interoperation::get_current_thread_id()
                )
            );

            return { nullptr, 0 };
        }

        char* pointer = static_cast<char*>(value);
        void* data = nullptr;
        std::uint64_t size{};

        std::memcpy(&size, pointer, sizeof(std::uint64_t));
        std::memcpy(&data, pointer + sizeof(std::uint64_t), sizeof(std::uint64_t));

        return { data, size };
    }

    // Each allocation is equipped with an allocation descriptor that contains:
    // 1. The size of the allocated memory.
    // 2. The base address of the allocated memory.
    // 3. A pointer to a cross-thread sharing counter.
    // When you use memory to initialize a new thread, the cross-thread sharing counter is incremented.
    // Another thing is that the new thread will get its own memory descriptor.
    // Without this descriptor, you won't be able to access the memory.
    // This means that you must call deallocate_program_memory twice (for each thread that shares the memory).
    // This makes memory sharing explicit and allows to verify it at runtime.
    module_mediator::memory allocate_program_memory(
        module_mediator::return_value thread_id,
        module_mediator::return_value thread_group_id, 
        module_mediator::eight_bytes size
    ) {
        module_mediator::return_value null_pointer = reinterpret_cast<module_mediator::return_value>(nullptr);
        module_mediator::return_value allocated_memory = interoperation::thread_group_allocate(
            thread_group_id, 
            size
        );

        if (allocated_memory == null_pointer) {
            return nullptr;
        }

        module_mediator::return_value value_pointer_data = interoperation::thread_allocate(
            thread_id,
            sizeof(std::uint64_t) * 3
        ); //first 8 bytes - allocated size, second 8 bytes - base address, third 8 bytes - pointer to thread-sharing counter

        if (value_pointer_data == null_pointer) {
            interoperation::thread_group_deallocate(
                thread_group_id,
                reinterpret_cast<module_mediator::memory>(allocated_memory)
            );

            return nullptr;
        }

        module_mediator::return_value cross_thread_sharing = interoperation::thread_group_allocate(
            thread_group_id,
            sizeof(std::uint64_t)
        );

        if (cross_thread_sharing == null_pointer) {
            interoperation::thread_group_deallocate(
                thread_group_id,
                reinterpret_cast<module_mediator::memory>(allocated_memory)
            );

            interoperation::thread_deallocate(
                thread_id, 
                reinterpret_cast<module_mediator::memory>(value_pointer_data)
            );

            return nullptr;
        }

        char* pointer_data = reinterpret_cast<char*>(value_pointer_data);

        //fill in memory size
        std::memcpy(pointer_data, &size, sizeof(std::uint64_t));

        //fill in actual memory value
        std::memcpy(pointer_data + sizeof(std::uint64_t), &allocated_memory, sizeof(std::uint64_t));

        //fill in cross-thread sharing pointer
        std::memcpy(pointer_data + sizeof(std::uint64_t) * 2, &cross_thread_sharing, sizeof(std::uint64_t));

        //initialize cross-thread sharing counter
        std::uint64_t thread_counter_initialization = 1;
        std::memcpy(
            reinterpret_cast<void*>(cross_thread_sharing),
            &thread_counter_initialization,
            sizeof(std::uint64_t)
        );

        return pointer_data;
    }

    void deallocate_program_memory(
        module_mediator::return_value thread_id,
        module_mediator::return_value thread_group_id, 
        module_mediator::memory address
    ) {
        if (address == nullptr) {
            return;
        }

        assert(
            interoperation::verify_thread_memory(interoperation::get_current_thread_id(), address) != module_mediator::module_failure
            && "Unexpected invalid memory for backend function"
        );

        //save memory's base address
        module_mediator::memory base{};
        std::memcpy(
            &base, 
            static_cast<char*>(address) + sizeof(std::uint64_t), 
            sizeof(std::uint64_t)
        );

        assert(base != nullptr && "Base address cannot be null");

        module_mediator::memory cross_thread_sharing{};
        std::memcpy(
            &cross_thread_sharing, 
            static_cast<char*>(address) + sizeof(std::uint64_t) * 2, 
            sizeof(std::uint64_t)
        );

        assert(cross_thread_sharing != nullptr && "Cross-thread sharing pointer cannot be null");
        std::atomic_ref cross_thread_sharing_synchronous = std::atomic_ref(
            *static_cast<std::uint64_t*>(cross_thread_sharing)
        );

        assert(cross_thread_sharing_synchronous.load(std::memory_order_seq_cst) > 0 && "Cross-thread sharing counter cannot be zero");
        if (cross_thread_sharing_synchronous.fetch_sub(1, std::memory_order_relaxed) == 1) {
            // Deallocate shared data and the memory itself.
            interoperation::thread_group_deallocate(thread_group_id, cross_thread_sharing);
            interoperation::thread_group_deallocate(thread_group_id, base);
        }

        // Deallocate the memory descriptor for the thread.
        interoperation::thread_deallocate(thread_id, address);
    }
}
