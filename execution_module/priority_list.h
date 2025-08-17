#ifndef PRIORITY_LIST_H
#define PRIORITY_LIST_H

#include "pch.h"

/// <summary>
/// A priority list is a data structure that stores elements with associated priorities.
/// The difference from a priority queue is that the priority list can look past the first element
/// and find the element with the highest priority that satisfies a certain condition.
/// </summary>
template<typename value_type, typename priority_type>
class priority_list {
private:
    // A questionable design choice, but I see no other way to ensure ordering based on keys,
    // while still allowing modification of the value_type.
    struct priority_node {
        mutable value_type value;
        priority_type priority;
    };

    struct priority_comparator {
        bool operator() (const priority_node& left, const priority_node& right) const noexcept {
            // Start from the highest priority
            // std::multiset is required to order elements with the same key in FIFO manner.
            // Notice that we do not know anything about value_type, so it is not a part of ordering.
            // In a language with a more meaningful type system, or a better implementation of the structure,
            // they would define something like a trait (type class, whatever you call it) that my object must
            // implement to create either partial or full ordering (depending on what std::multiset wants) that
            // the structure will use, instead of just making access through iterator be const.
            // This is actually ridiculous, just read https://cplusplus.github.io/LWG/issue103, the "rationale" section.
            return left.priority > right.priority;
        }
    };

public:
    class proxy {
    private:
        using iterator_type = std::multiset<priority_node, priority_comparator>::iterator;
        proxy(iterator_type elem)
            :associated_element{ new iterator_type{ std::move(elem) }}
        {}

        iterator_type* associated_element;
        void free_resource() noexcept {
            delete this->associated_element;
            this->associated_element = nullptr;
        }

    public:
        proxy() noexcept
            :associated_element{ nullptr }
        {}

        proxy(const proxy&) = delete;
        void operator=(const proxy&) = delete;

        proxy(proxy&& other) noexcept
            :associated_element{ other.associated_element }
        {
            other.associated_element = nullptr;
        }

        proxy& operator=(proxy&& other) noexcept {
            this->free_resource();

            this->associated_element = other.associated_element;
            other.associated_element = nullptr;

            return *this;
        }

        value_type& operator* () noexcept {
            assert(this->associated_element && "invalid priority_list proxy used");
            return (*this->associated_element)->value;
        }

        value_type* operator-> () noexcept {
            assert(this->associated_element && "invalid priority_list proxy used");
            return &(*this->associated_element)->value;
        }

        const value_type& operator* () const noexcept {
            assert(this->associated_element && "invalid priority_list proxy used");
            return (*this->associated_element)->value;
        }

        const value_type* operator-> () const noexcept {
            assert(this->associated_element && "invalid priority_list proxy used");
            return &(*this->associated_element)->value;
        }

        bool has_resource() const noexcept {
            return this->associated_element != nullptr;
        }

        ~proxy() noexcept {
            this->free_resource();
        }

        friend class priority_list;
    };

private:
    std::multiset<priority_node, priority_comparator> type_priorities{ priority_comparator{} };

public:
    template<typename predicate_type>
    std::pair<value_type*, priority_type> find(predicate_type predicate)
        noexcept(
            noexcept(
                predicate(std::declval<value_type&>())
            )
        )
    {
        for (auto iterator = this->type_priorities.begin(), end = this->type_priorities.end();
            iterator != end; ++iterator) {
            if (predicate(iterator->value)) {
                return std::pair{ &iterator->value, iterator->priority };
            }
        }

        return std::pair{ nullptr, priority_type{} };
    }

    std::size_t count() const noexcept { return this->type_priorities.size(); }
    void remove(proxy proxy) noexcept { this->type_priorities.erase(*proxy.associated_element); }

    template<typename... arguments_type>
    proxy push(priority_type priority, arguments_type&&... values) {
        // Previous implementation used std::list to store elements, but std::multiset
        // is more suitable for this case, as it already can sort elements by priority,
        // while maintaining FIFO order for elements with the same priority.
        // Moreover, std::multiset insert operation has logarithmic complexity.
        // Another thing is that std::multiset does not invalidate iterators (just like std::list),
        // unless the element is removed.

        // I use C++20 and onward, so "value_type" prvalue shouldn't even materialize here.
        return proxy{
            this->type_priorities.emplace(
                value_type{ std::forward<arguments_type>(values)... }, priority
            )
        };
    }
};

#endif // !PRIORITY_LIST_H