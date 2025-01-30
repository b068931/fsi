#ifndef PRIORITY_LIST_H
#define PRIORITY_LIST_H

#include "pch.h"

template<typename T, typename priority_type>
class priority_list {
public:
	class proxy {
	private:
		using iterator_type = typename std::list<std::pair<T, priority_type>>::iterator;

		typename iterator_type* associated_element;
		void free_resource() noexcept {
			delete this->associated_element;
			this->associated_element = nullptr;
		}

	public:
		proxy()
			:associated_element{ nullptr }
		{}

		proxy(iterator_type elem)
			:associated_element{ new iterator_type{ std::move(elem) }}
		{}

		proxy(const proxy&) = delete;
		void operator=(const proxy&) = delete;

		proxy(proxy&& pr) noexcept
			:associated_element{ pr.associated_element }
		{
			pr.associated_element = nullptr;
		}
		void operator=(proxy&& pr) noexcept {
			this->free_resource();

			this->associated_element = pr.associated_element;
			pr.associated_element = nullptr;
		}

		T& operator* () {
			assert(this->associated_element && "invalid priority_list proxy used");
			return (*this->associated_element)->first;
		}
		T* operator-> () {
			assert(this->associated_element && "invalid priority_list proxy used");
			return &((*this->associated_element)->first);
		}

		~proxy() {
			this->free_resource();
		}

		friend class priority_list<T, priority_type>;
	};

private:
	std::list<std::pair<T, priority_type>> type_priorities;
	auto get_iterator(std::size_t displacement) {
		auto top = this->type_priorities.rbegin();
		std::advance(top, displacement);

		return top;
	}

public:
	template<typename F>
	std::pair<T*, priority_type> find(F predicate) {
		for (auto iterator = this->type_priorities.rbegin(), end = this->type_priorities.rend();
			iterator != end; ++iterator) {
			if (predicate(iterator->first)) {
				return std::pair{&iterator->first, iterator->second};
			}
		}

		return std::pair{nullptr, priority_type{}};
	}

	std::size_t count() const { return this->type_priorities.size(); }
	void remove(proxy proxy) { this->type_priorities.erase(*(proxy.associated_element)); }

	template<typename... args>
	proxy push(priority_type priority, args&&... values) {
		if (
			(this->count() == 0) || (this->type_priorities.back().second < priority)
		) { //special case for adding threads with the highest priority
			this->type_priorities.push_back(
				std::pair<T, priority_type>{ T{ std::forward<args>(values)... }, priority }
			);
			
			auto last_element_iterator = this->type_priorities.end();
			return proxy{ std::prev(last_element_iterator) };
		}
		else {
			for (auto iterator = this->type_priorities.begin(), end = this->type_priorities.end(); 
				 iterator != end; ++iterator) {
				if (iterator->second >= priority) {
					return proxy{ 
						this->type_priorities.insert(
							iterator, 
							std::pair<T, priority_type>{ T{ std::forward<args>(values)... }, priority }
						)
					};
				}
			}
		}

		return proxy{};
	}
};

#endif // !PRIORITY_LIST_H