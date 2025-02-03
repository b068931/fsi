#include "pch.h"
#include "resource_module.h"
#include "program_container.h"
#include "thread_structure.h"
#include "id_generator.h"
#include "module_interoperation.h"
#include "../console_and_debug/logging.h"

//remove "#define I_HATE_MICROSOFT_AND_STUPID_ANALYZER_WARNINGS" to disable _Acquires_lock_ and _Releases_lock_
#define I_HATE_MICROSOFT_AND_STUPID_ANALYZER_WARNINGS
#ifndef I_HATE_MICROSOFT_AND_STUPID_ANALYZER_WARNINGS
	#define _Acquires_lock_(a)
	#define _Releases_lock_(a)
#endif

enum class return_code : module_mediator::return_value {
	ok,
	concurrency_error
};
class index_getter {
public:
	static std::size_t excm() {
		static std::size_t index = get_module_part()->find_module_index("excm");
		return index;
	}

	static std::size_t excm_on_container_creation() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "on_container_creation");
		return index;
	}

	static std::size_t excm_on_thread_creation() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::excm(), "on_thread_creation");
		return index;
	}

	static std::size_t progload() {
		static std::size_t index = get_module_part()->find_module_index("progload");
		return index;
	}

	static std::size_t progload_free_program() {
		static std::size_t index = get_module_part()->find_function_index(index_getter::progload(), "free_program");
		return index;
	}
};

id_generator::id_type id_generator::current_id = 1;
std::stack<id_generator::id_type> id_generator::free_ids{};
std::mutex id_generator::lock{};

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
		get_module_part(), 
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
void add_destory_callback_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id, void(*callback)(void*), void* paired_information) {
	auto iterator_lock = get_iterator(object, mutex, id);
	if (iterator_lock.second) {
		iterator_lock.first->second.destroy_callbacks.push_back(std::pair{callback, paired_information});
	}
}

template<typename T>
intptr_t allocate_memory_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id, std::uint64_t size) {
	auto iterator_lock = get_iterator(object, mutex, id);

	/*
	* in theory, this operation should always be successful
	* because program container can not be destroyed while it has at least one running thread,
	* and the memory belonging to a program can be allocated only by its own threads.
	* so "bool(iterator_lock.second)" works mostly as a precaution.
	*/

	if (iterator_lock.second) { //check if we acquired mutex for an object
		iterator_lock.first->second.allocated_memory.push_back(new(std::nothrow) char[size] {});
		return reinterpret_cast<std::uintptr_t>(iterator_lock.first->second.allocated_memory.back());
	}
	
	LOG_PROGRAM_WARNING(get_module_part(), "Concurrency error.");

	return reinterpret_cast<std::uintptr_t>(nullptr);
	_Releases_lock_(iterator_lock->second);
}

template<typename T>
void deallocate_memory_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id, void* address) {
	//see allocate_memory_generic
	auto iterator_lock = get_iterator(object, mutex, id);
	if (iterator_lock.second) {
		std::vector<void*>& allocated_memory = iterator_lock.first->second.allocated_memory;
		auto found_address = std::find(
			allocated_memory.begin(),
			allocated_memory.end(),
			address
		);

		//if address does not belong to this structure we do nothing
		if (found_address != allocated_memory.end()) {
			delete[] *found_address;
			allocated_memory.erase(found_address);
		}
		else {
			LOG_PROGRAM_WARNING(get_module_part(), "Deallocated memory does not belong to the object.");
		}
	}
}

template<typename T>
std::conditional_t<
	std::is_same_v<T, std::map<id_generator::id_type, thread_structure>>, //for thread_structure this function also returns id of the asscociated program_container
	std::pair<return_code, id_generator::id_type>,
	return_code
> 
	deallocate_generic(std::recursive_mutex& mutex, T& object, id_generator::id_type id) {
	constexpr bool thread_structure_switch = std::is_same_v<T, std::map<id_generator::id_type, thread_structure>>;

	[[maybe_unused]] id_generator::id_type program_container_id = 0;
	id_generator::id_type free_id = 0;
	{
		std::unique_lock lock{ mutex };
		auto iterator_lock = get_iterator(object, mutex, id);

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

	if(free_id != 0) {
		id_generator::free_id(free_id);
		
		if constexpr (thread_structure_switch) {
			return std::pair{ return_code::ok, program_container_id };
		}
		else {
			return return_code::ok;
		}
	}

	if constexpr (thread_structure_switch) {
		LOG_PROGRAM_WARNING(get_module_part(), "Concurrency error.");
		return std::pair{ return_code::concurrency_error, 0 };
	}
	else {
		LOG_PROGRAM_WARNING(get_module_part(), "Concurrency error.");
		return return_code::concurrency_error;
	}
}

void insert_new_container(id_generator::id_type id, program_context* context) {
	program_container new_container{};
	new_container.context = context;

	std::lock_guard lock{ containers_mutex };
	containers[id] = std::move(new_container);
}
module_mediator::return_value notify_excm_new_container(
	id_generator::id_type id, 
	void* main_function, 
	std::uint64_t preferred_stack_size
) {
	return module_mediator::fast_call<module_mediator::return_value, void*>(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_on_container_creation(),
		id,
		main_function,
		preferred_stack_size
	);
}

module_mediator::return_value add_container_on_destroy(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*>(bundle);
	add_destory_callback_generic(
		containers_mutex,
		containers,
		std::get<0>(arguments),
		static_cast<void(*)(void*)>(std::get<1>(arguments)),
		std::get<2>(arguments)
	);

	return 0;
}
module_mediator::return_value add_thread_on_destroy(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<module_mediator::return_value, void*, void*>(bundle);
	add_destory_callback_generic(
		thread_structures_mutex,
		thread_structures,
		std::get<0>(arguments),
		static_cast<void(*)(void*)>(std::get<1>(arguments)),
		std::get<2>(arguments)
	);

	return 0;
}

module_mediator::return_value duplicate_container(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type, void*>(bundle);
	id_generator::id_type container_id = std::get<0>(arguments);
	void* main_function = std::get<1>(arguments);

	auto iterator_lock = get_iterator(containers, containers_mutex, container_id);
	if (iterator_lock.second) {
		id_generator::id_type new_container_id = id_generator::get_id();
		insert_new_container(
			new_container_id,
			program_context::duplicate(iterator_lock.first->second.context)
		);

		iterator_lock.second.unlock();

		LOG_PROGRAM_INFO(get_module_part(), "Duplicating the program context.");
		return notify_excm_new_container(
			new_container_id,
			main_function,
			iterator_lock.first->second.context->preferred_stack_size
		);
	}

	return 1;
}
module_mediator::return_value get_preferred_stack_size(module_mediator::arguments_string_type bundle) {
	constexpr std::uint64_t fallback_stack_size = 1024;

	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto iterator_lock = get_iterator(containers, containers_mutex, std::get<0>(arguments));
	if (iterator_lock.second) {
		return iterator_lock.first->second.context->preferred_stack_size;
	}

	LOG_PROGRAM_WARNING(
		get_module_part(),
		std::format("Using fallback stack size of {}.", std::get<0>(arguments), fallback_stack_size)
	);

	return fallback_stack_size;
}

module_mediator::return_value create_new_program_container(module_mediator::arguments_string_type bundle) {
	id_generator::id_type id = id_generator::get_id();
	auto arguments = module_mediator::arguments_string_builder::unpack<
		std::uint64_t, 
		std::uint32_t, 
		void*, 
		std::uint32_t, 
		void*, 
		std::uint32_t, 
		void*, 
		std::uint64_t, 
		void*, 
		std::uint64_t
	>(bundle);

	std::uint64_t preferred_stack_size = std::get<0>(arguments);
	std::uint32_t main_function_index = std::get<1>(arguments);
	
	void** code = static_cast<void**>(std::get<2>(arguments));
	std::uint32_t functions_count = std::get<3>(arguments);

	void** exposed_functions = static_cast<void**>(std::get<4>(arguments));
	std::uint32_t exposed_functions_count = std::get<5>(arguments);

	void* jump_table = std::get<6>(arguments);
	std::uint64_t jump_table_size = std::get<7>(arguments);

	void** program_strings = static_cast<void**>(std::get<8>(arguments));
	std::uint64_t program_strings_size = std::get<9>(arguments);
	
	/*
	* a newly created object can not be deleted or modified if excm does not know about it,
	* so we don't need to lock on its mutex
	*/

	insert_new_container(
		id,
		program_context::create(
			preferred_stack_size,
			code, functions_count, 
			exposed_functions, exposed_functions_count, 
			jump_table, jump_table_size,
			program_strings, program_strings_size
		)
	);

	return notify_excm_new_container(id, code[main_function_index], preferred_stack_size);
}
module_mediator::return_value create_new_thread(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	std::uint64_t preferred_stack_size{};

	id_generator::id_type container_id = std::get<0>(arguments);
	id_generator::id_type id = id_generator::get_id();

	/*
	* despite the fact that get_iterator can fail,
	* it should never happen in this case because a new thread can be created only by an already existing thread,
	* and creating thread can not continue its execution until a new thread is created
	*/

	{
		auto iterator_lock = get_iterator(containers, containers_mutex, container_id);
		assert(iterator_lock.first->first == container_id);

		std::lock_guard lock{ thread_structures_mutex };
		thread_structures[id] = thread_structure{ iterator_lock.first->first };

		preferred_stack_size = iterator_lock.first->second.context->preferred_stack_size;
		++iterator_lock.first->second.threads_count;
	}

	return module_mediator::fast_call<module_mediator::return_value, module_mediator::return_value>(
		get_module_part(),
		index_getter::excm(),
		index_getter::excm_on_thread_creation(),
		container_id,
		id,
		preferred_stack_size
	);
}

module_mediator::return_value allocate_program_memory(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type, std::uint64_t>(bundle);
	return allocate_memory_generic(containers_mutex, containers, std::get<0>(arguments), std::get<1>(arguments));
}
module_mediator::return_value allocate_thread_memory(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type, std::uint64_t>(bundle);
	return allocate_memory_generic(thread_structures_mutex, thread_structures, std::get<0>(arguments), std::get<1>(arguments));
}

module_mediator::return_value deallocate_program_memory(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type, void*>(bundle);
	deallocate_memory_generic(containers_mutex, containers, std::get<0>(arguments), std::get<1>(arguments));

	return 0;
}
module_mediator::return_value deallocate_thread_memory(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type, void*>(bundle);
	deallocate_memory_generic(thread_structures_mutex, thread_structures, std::get<0>(arguments), std::get<1>(arguments));

	return 0;
}

module_mediator::return_value deallocate_program_container(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	return static_cast<module_mediator::return_value>(deallocate_generic(containers_mutex, containers, std::get<0>(arguments)));
}
module_mediator::return_value deallocate_thread(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto code_id = deallocate_generic(thread_structures_mutex, thread_structures, std::get<0>(arguments));
	if (code_id.first != return_code::concurrency_error) {
		auto iterator_lock = get_iterator(containers, containers_mutex, code_id.second);
		--iterator_lock.first->second.threads_count;

		return code_id.second;
	}

	return 0;
}

module_mediator::return_value get_running_threads_count(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto iterator_lock = get_iterator(containers, containers_mutex, std::get<0>(arguments));
	if (iterator_lock.second) {
		return iterator_lock.first->second.threads_count;
	}

	/*
	* ::max() means that the requested program container does not exist.
	* we don't return 0 because an index to this program container might have already been reused
	*/

	return std::numeric_limits<module_mediator::return_value>::max(); 
}
module_mediator::return_value get_program_container_id(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto iterator_lock = get_iterator(thread_structures, thread_structures_mutex, std::get<0>(arguments));
	if (iterator_lock.second) {
		iterator_lock.first->second.program_container;
	}

	//ids start from 1
	return 0;
}

module_mediator::return_value get_jump_table(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto iterator_lock = get_iterator(containers, containers_mutex, std::get<0>(arguments));
	if (iterator_lock.second) {
		return reinterpret_cast<std::uintptr_t>(iterator_lock.first->second.context->jump_table);
	}
	
	return reinterpret_cast<std::uintptr_t>(nullptr);
}
module_mediator::return_value get_jump_table_size(module_mediator::arguments_string_type bundle) {
	auto arguments = module_mediator::arguments_string_builder::unpack<id_generator::id_type>(bundle);
	auto iterator_lock = get_iterator(containers, containers_mutex, std::get<0>(arguments));
	if (iterator_lock.second) {
		return iterator_lock.first->second.context->jump_table_size;
	}

	return 0;
}

program_container::~program_container() noexcept {
	if (this->context && (this->context->decrease_references_count() == 0)) {
		delete this->context;
	}

	assert(this->threads_count == 0 && "destroying container while it still has running threads");
	//excm will make sure that there are no active threads before destorying a program container
}
program_context::~program_context() noexcept {
	assert(this->references_count == 0);
	module_mediator::fast_call<void*, std::uint32_t, void*, std::uint32_t, void*, void*, std::uint64_t>(
		get_module_part(),
		index_getter::progload(),
		index_getter::progload_free_program(),
		this->code,
		this->functions_count,
		this->exposed_functions,
		this->exposed_functions_count,
		this->jump_table,
		this->program_strings,
		this->program_strings_size
	);
}