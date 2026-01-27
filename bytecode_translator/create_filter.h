#ifndef CREATE_FILTER_H
#define CREATE_FILTER_H

#include "structure_builder.h"
#include "translator_error_type.h"
#include "../submodule_typename_array/typename-array/typename-array-primitives/include-all-namespace.hpp"

template<typename... filters>
struct create_filter {
private:
    // If no filters were specified (or we went through all of them), then just add a default implementation that always returns true.
    template<typename>
    struct filter_wrapper {
        translator_error_type error_message;
        bool check(const structure_builder::instruction&) {
            return true;
        }
    };

    template<template<typename...> class wrapper, typename filter, typename... other>
    struct filter_wrapper<wrapper<filter, other...>> : public filter_wrapper<wrapper<other...>> {
        using base_class = filter_wrapper<typename_array_primitives::typename_array<other...>>;
        bool check(const structure_builder::instruction& instruction) {
            if (this->base_class::check(instruction)) { //at first we use base class filters
                if (!filter::check(instruction)) { //and only after that we apply our current filter. this way we will not modify error message if one of the filters higher in class hierarchy returns false
                    this->error_message = filter::error_message;
                    return false;
                }

                return true;
            }

            return false;
        }
    };

public:
    using type = filter_wrapper<typename_array_primitives::typename_array<filters...>>;
};

#endif
