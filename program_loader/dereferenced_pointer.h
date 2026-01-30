#ifndef DEREFERENCED_POINTER_H
#define DEREFERENCED_POINTER_H

#include "pch.h"
#include "variable_with_id.h"

class dereferenced_pointer : public variable_with_id {
    std::uint8_t active_type;
    std::vector<entity_id> dereference_indexes;

public:
    dereferenced_pointer(
        std::uint8_t pointer_active_type, 
        entity_id pointer_id, 
        std::vector<entity_id>&& pointer_dereference_indexes
    )
        :variable_with_id{ pointer_id },
        active_type{ pointer_active_type },
        dereference_indexes{ std::move(pointer_dereference_indexes) }
    {}

    std::uint8_t get_active_type() const { return this->active_type; }
    const std::vector<entity_id>& get_dereference_indexes() const { return this->dereference_indexes; }
    void visit(instruction_builder* builder) override;
};

#endif // !DEREFERENCED_POINTER_H
