#ifndef TYPENAME_ARRAY_H
#define TYPENAME_ARRAY_H

#include <type_traits>
//typename_array is a header that is used to work with types in templates

using typename_array_size_t = long long;
constexpr typename_array_size_t npos = -1;

template<typename... args>
struct typename_array {
	static constexpr typename_array_size_t size = sizeof... (args);

	template<template<typename... other> class templ>
	using acquire = templ<args...>;
};

template<typename array_1, typename array_2>
struct combine;

template<template<typename...> class array_template, typename ...args_1, typename ...args_2>
struct combine<array_template<args_1...>, array_template<args_2...>> {
	using new_array = array_template<args_1..., args_2...>;
};

template<typename_array_size_t index, typename array>
struct get {
private:
	template<typename_array_size_t counter, typename array>
	struct get_helper;

	template<template<typename, typename...> class array_template, typename val, typename... other>
	struct get_helper<0, array_template<val, other...>> {
		using value = val;
	};

	template<typename_array_size_t counter, template<typename, typename...> class array_template, typename val, typename... other>
	struct get_helper<counter, array_template<val, other...>> {
		using value = typename get_helper<counter - 1, typename_array<other...>>::value;
	};

public:
	using value = typename get_helper<index, array>::value;
};

template<typename array, typename val_to_find>
struct find {
private:
	template<typename array, typename find, typename_array_size_t index>
	struct find_helper {
		static constexpr typename_array_size_t indx = npos;
	};

	template<template<typename, typename...> class array_template, typename val, typename... other, typename find, typename_array_size_t index>
	struct find_helper<array_template<val, other...>, find, index> {
		static constexpr typename_array_size_t indx = find_helper<array_template<other...>, find, index + 1>::indx;
	};

	template<template<typename, typename...> class array_template, typename... other, typename find, typename_array_size_t index>
	struct find_helper<array_template<find, other...>, find, index> {
		static constexpr typename_array_size_t indx = index;
	};

public:
	static constexpr typename_array_size_t index = find_helper<array, val_to_find, 0>::indx;
};

template<auto val>
struct value {
	static constexpr decltype(val) get_value = val;
};

template<typename array, template<typename, typename_array_size_t> class funct>
struct apply {
private:
	template<typename array, template<typename, typename_array_size_t> class funct, typename_array_size_t index>
	struct apply_helper {
		using new_array = typename_array<>;
	};

	template<template<typename> class templ, typename value, template<typename, typename_array_size_t> class funct, typename_array_size_t index>
	struct apply_helper<templ<value>, funct, index> {
		using new_array = typename_array<typename funct<value, index>::new_value>;
	};

	template<template<typename, typename...> class templ, typename... other, typename value, template<typename, typename_array_size_t> class funct, typename_array_size_t index>
	struct apply_helper<templ<value, other...>, funct, index> {
		using new_array = typename combine<
			typename_array<typename funct<value, index>::new_value>,
			typename apply_helper<templ<other...>, funct, (index + 1)>::new_array
		>::new_array;
	};

public:
	using new_array = typename apply_helper<array, funct, 0>::new_array;
};

template<typename array, template<typename> class funct, typename return_type>
struct sum {
private:
	template<typename array, template<typename> class funct>
	struct sum_helper {
		static constexpr return_type new_value{ 0 };
	};

	template<template<typename> class templ, typename value, template<typename> class funct>
	struct sum_helper<templ<value>, funct> {
		static constexpr return_type new_value = funct<value>::value;
	};

	template<template<typename, typename...> class templ, typename value, typename... other, template<typename> class funct>
	struct sum_helper<templ<value, other...>, funct> {
		static constexpr decltype(auto) new_value = funct<value>::value + sum_helper<templ<other...>, funct>::new_value;
	};

public:
	static constexpr decltype(auto) new_value = sum_helper<array, funct>::new_value;
};

#endif