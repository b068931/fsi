#ifndef CLOCK_LIST_H
#define CLOCK_LIST_H

#include "pch.h"

/*
* a linked list in which the last element points to the first one,
* creating a structure that can be used to iterate through all elements over and over
*/

/// <summary>
/// A clock list is a data structure that allows for circular iteration over its elements.
template<typename T>
class clock_list {
private:
	struct list_element {
		T object{};

		list_element* next{};
		list_element* previous{};
	};

	list_element* hand{ nullptr };
	std::size_t elements_count{ 0 };

public:
	class proxy {
	private:
		list_element* associated_element;

	public:
		proxy()
			:associated_element{ nullptr }
		{}

		proxy(list_element* elem)
			:associated_element{elem}
		{}

		proxy(const proxy&) = delete;
		void operator= (const proxy&) = delete;

		proxy(proxy&&) noexcept = default;
		proxy& operator= (proxy&&) noexcept = default;

		T& operator* () {
			return this->associated_element->object;
		}

		T* operator-> () {
			return &this->associated_element->object;
		}

		friend class clock_list;
	};

public:
	T* get_current() const noexcept {
		if (this->hand) {
			return &this->hand->object;
		}

		return nullptr;
	}

	void remove(proxy proxy) noexcept {
		assert(this->elements_count != 0);

		list_element* to_delete = proxy.associated_element;
		if (this->elements_count == 1) { //special case if only one element exists
			delete to_delete;
			this->hand = nullptr;
		}
		else {
			if (to_delete == this->hand) {
				list_element* new_hand = this->hand->next;
				this->hand = new_hand;
			}

			to_delete->next->previous = to_delete->previous;
			to_delete->previous->next = to_delete->next;

			delete to_delete;
		}

		--this->elements_count;
	}

	template<typename... args>
	proxy push_after(args&&... values) {
		list_element* new_element = new list_element{
		    .object = T{ std::forward<args>(values)... },
		    .next = nullptr,
		    .previous = nullptr
		};

		if (this->elements_count == 0) {
			this->hand = new_element;
			
			new_element->next = new_element;
			new_element->previous = new_element;
		}
		else {
			new_element->previous = this->hand;
			new_element->next = this->hand->next;

			this->hand->next->previous = new_element;
			this->hand->next = new_element;
		}

		++this->elements_count;
		return proxy{ new_element };
	}

	void make_step() noexcept {
		assert(this->elements_count != 0);
		this->hand = this->hand->next;
	}

	std::size_t get_elements_count() const noexcept { return this->elements_count; }
};

#endif // !CLOCK_LIST_H
