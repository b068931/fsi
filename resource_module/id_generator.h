#ifndef RESOURCE_MODULE_ID_GENERATOR_H
#define RESOURCE_MODULE_ID_GENERATOR_H

#include "pch.h"
#include "../module_mediator/module_part.h"

class id_generator {
public:
    using id_type = module_mediator::return_value;

private:
#ifndef NDEBUG // We must ensure specific ordering when freeing objects. Structures must get fully destroyed in execution module before freeing them in resource module
    inline static std::stack<id_type> free_ids{};
    inline static std::mutex lock{};
    inline static id_type current_id = 1;
#else // For release builds, there is no point doing that. This is faster, and yet it is still impossible to run out of IDs
    inline static std::atomic<id_type> current_id{ 1 };
#endif

public:
    static id_type get_id() {
#ifndef NDEBUG
        std::lock_guard scope{ lock };
        if (!free_ids.empty()) {
            id_type id = free_ids.top();
            free_ids.pop();

            return id;
        }

        return current_id++;
#else
        return current_id.fetch_add(1, std::memory_order_relaxed);
#endif
    }
    static void free_id(id_type id) {
#ifndef NDEBUG
        std::lock_guard scope{ lock };
        free_ids.push(id);
        assert(id != 0);
#else
        (void(id));
#endif
    }
};

#endif
