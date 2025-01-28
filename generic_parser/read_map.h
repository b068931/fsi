#ifndef GENERIC_PARSER_READ_MAP_H
#define GENERIC_PARSER_READ_MAP_H

#include <utility> //std::pair std::forward
#include <string>
#include <map>
#include <any>
#include <stdexcept> //std::out_of_range
#include <vector>
#include <algorithm> //std::find
#include <functional> //std::function
#include <stack>
#include <cassert>
#include <memory> //std::unique_ptr
#include <unordered_map>
#include <forward_list>
#include <type_traits>

#include "token_generator.h"

namespace generic_parser {
	enum class state_action {
		no_action,
		push_state,
		change_top,
		pop_top
	};

	template<typename parameters_enumeration_type>
	class parameters_container {
	private:
		std::unordered_map<parameters_enumeration_type, std::any> parameters;
		bool contains_parameter(parameters_enumeration_type key) {
			return this->parameters.count(key) == 1;
		}

	public:
		parameters_container() = default;

		parameters_container(const parameters_container&) = delete;
		parameters_container& operator=(const parameters_container&) = delete;

		parameters_container(parameters_container&&) = delete;
		parameters_container& operator=(parameters_container&&) = delete;

		template<typename T>
		parameters_container& assign_parameter(parameters_enumeration_type key, T&& value) {
			this->parameters[key] = std::forward<T>(value);
			return *this;
		}
		parameters_container& remove_parameter(parameters_enumeration_type key) {
			this->parameters.erase(key);
			return *this;
		}

		template<typename T>
		T& retrieve_parameter(parameters_enumeration_type key) {
			if (this->contains_parameter(key)) {
				return std::any_cast<T&>(this->parameters[key]);
			}

			throw std::out_of_range{ "parameter with specified key does not exist" };
		}
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class read_map;

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class states_builder;

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class state;

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class state_settings;

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class state_base {
	private:
		template<typename parameters_object>
		struct get_function_signature {
			using value = bool(parameters_container<parameters_enumeration_type>&, parameters_object&);
		};

		template<>
		struct get_function_signature<void> {
			using value = bool(parameters_container<parameters_enumeration_type>&);
		};

	protected:
		using redirection_information_type =
			std::pair<state<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>*, state_action>;

		using custom_function_signature = typename get_function_signature<parameters_object>::value;

	private:
		bool detached_name{ true };
		bool all_tokens_handler{ false };
		bool whitelist{ true };

		std::string error_message{ "Unknown error" };

		std::vector<token_type> handle_tokens;
		std::vector<
			std::pair<std::function<custom_function_signature>, redirection_information_type>
		> custom_functions;
		std::map<token_type, redirection_information_type> redirections_for_tokens;

	protected:
		template<typename... types>
		redirection_information_type inner_get_redirection_from_custom_functions(types&&... values) {
			for (auto& function_redirection_pair : this->custom_functions) {
				if (function_redirection_pair.first(std::forward<types>(values)...)) {
					return function_redirection_pair.second;
				}
			}

			return redirection_information_type{ nullptr, state_action::no_action };
		}

	public:
		bool is_handle_token(token_type token) {
			auto found_token = std::find(this->handle_tokens.cbegin(), this->handle_tokens.cend(), token);
			return (found_token != this->handle_tokens.cend()) || this->all_tokens_handler;
		}

		redirection_information_type get_redirection_for_token(
			token_type token
		) {
			auto found_redirection = this->redirections_for_tokens.find(token);
			if (found_redirection != this->redirections_for_tokens.end()) {
				return found_redirection->second;
			}

			return redirection_information_type{ nullptr, state_action::no_action };
		}

		bool is_detached_name() {
			return this->detached_name;
		}
		bool is_whitelist() {
			return this->whitelist;
		}
		virtual std::string get_associated_error() {
			return this->error_message;
		}

		virtual ~state_base() = default;
		friend class state_settings<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;
		friend class states_builder<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class state : public state_base<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type> {
	private:
		using base = state_base<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;

	public:
		virtual void handle_token(
			objects& file_objects,
			parameters_object& parameters,
			read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>& read_map
		) {
			//this function was intentionally left empty
		}

		typename base::redirection_information_type get_redirection_from_custom_functions(
			parameters_container<parameters_enumeration_type>& parameters,
			parameters_object& parameters_object
		) {
			return this->inner_get_redirection_from_custom_functions(parameters, parameters_object);
		}
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_enumeration_type>
	class state<token_type, context_key_type, objects, void, parameters_enumeration_type> : public state_base<token_type, context_key_type, objects, void, parameters_enumeration_type> {
	private:
		using base = state_base<token_type, context_key_type, objects, void, parameters_enumeration_type>;

	public:
		virtual void handle_token(
			objects& file_objects,
			read_map<token_type, context_key_type, objects, void, parameters_enumeration_type>& read_map
		) {
			//this function was intentionally left empty
		}

		typename base::redirection_information_type get_redirection_from_custom_functions(
			parameters_container<parameters_enumeration_type>& parameters
		) {
			return this->inner_get_redirection_from_custom_functions(parameters);
		}
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class read_map_base {
	protected:
		using state_type = state<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;
		using parameters_container_type = parameters_container<parameters_enumeration_type>;
		using redirection_information_type = std::pair<state_type*, state_action>;

	private:
		token_type name_token;
		std::vector<std::unique_ptr<state_type>> states;

		token_generator<token_type, context_key_type>* generator;
		parameters_container_type parameters;

		std::string error_message;

		std::unique_ptr<std::string> detached_name;
		bool is_additional_token;

	protected:
		std::stack<state_type*> states_stack;

		token_type current_token;
		token_type end_token;

		bool working;
		bool handle_error_exit;

		void set_state_error_message(state_type* state) {
			this->error_message = state->get_associated_error();
		}

		template<typename... types>
		void call_handler(state_type* state, types... values) {
			if (state->is_handle_token(this->get_current_token())) {
				this->working = true;
				state->handle_token((*values)...);
			}
		}
		template<typename... types>
		void redirect_to_another_state(state_type* state, types... values) {
			redirection_information_type redirection = this->get_redirection(state, values...);
			if (redirection.second != state_action::no_action) {
				this->working = true;
				this->choose_action(redirection);
			}
		}
		void check_state_behaviour(state_type* state) {
			if ((!this->working) && state->is_detached_name()) {
				//detached name can not be used for the last token
				if ((this->get_current_token() == this->name_token) || this->generator->is_token_from_names_stack()) {
					this->working = true;
					this->detached_name.reset(new std::string{ this->generator->get_name() });

					this->is_additional_token = this->generator->is_token_from_names_stack();
				}
				else {
					this->set_state_error_message(state);
				}
			}
			else if ((!this->working) && state->is_whitelist()) {
				this->set_state_error_message(state);
			}
			else {
				this->working = true;
				if (this->detached_name) {
					this->detached_name.reset(nullptr);
					this->is_additional_token = false;
				}
			}

			this->working = this->is_working() && (this->get_current_token() != this->end_token);
		}
		bool initialize_handle(token_type token, state_type* current_state) {
			this->current_token = token;
			this->working = false;

			assert(this->states_stack.size() != 0 && "Empty states stack.");
			if ((this->detached_name.get()) != nullptr && (!this->generator->is_name_empty())) {
				this->set_state_error_message(current_state);
				return false;
			}

			return true;
		}

	private:
		template<typename... types>
		redirection_information_type get_redirection(state_type* state, types... values) {
			redirection_information_type custom_functions_redirection =
				state->get_redirection_from_custom_functions((*values)...);

			if (custom_functions_redirection.second == state_action::no_action) {
				redirection_information_type token_redirection =
					state->get_redirection_for_token(this->get_current_token());

				if (token_redirection.second != state_action::no_action) {
					return token_redirection;
				}
			}
			else {
				return custom_functions_redirection;
			}

			return redirection_information_type{ nullptr, state_action::no_action };
		}
		void choose_action(const redirection_information_type& redirection) {
			switch (redirection.second) {
			case state_action::push_state:
				this->states_stack.push(redirection.first);
				break;

			case state_action::change_top:
				this->states_stack.top() = redirection.first;
				break;

			case state_action::pop_top:
				assert(redirection.first == nullptr);
				this->states_stack.pop();

				break;
			}
		}

	public:
		read_map_base(token_type end_token, token_type name_token, token_generator<token_type, context_key_type>* token_generator)
			:end_token{ end_token },
			name_token{ name_token },
			current_token{ end_token },
			handle_error_exit{ false },
			working{ false },
			generator{ token_generator },
			detached_name{ nullptr },
			is_additional_token{ false }
		{
		}

		read_map_base(const read_map_base&) = delete;
		read_map_base& operator=(read_map_base&) = delete;

		read_map_base(read_map_base&&) = delete;
		read_map_base& operator=(read_map_base&&) = delete;

		parameters_container_type& get_parameters_container() {
			return this->parameters;
		}
		token_type get_current_token() {
			return this->current_token;
		}

		const std::string& error() const {
			return this->error_message;
		}
		void exit_with_error(std::string&& message) {
			this->handle_error_exit = true;
			this->working = false;

			this->error_message = std::move(message);
		}
		void exit_with_error() {
			this->handle_error_exit = true;
			this->working = false;

			this->error_message = this->states_stack.top()->get_associated_error();
		}

		std::string get_token_generator_name() {
			if (this->detached_name) {
				std::string saved_name{ std::move(*this->detached_name) };
				return saved_name;
			}

			return this->generator->get_name();
		}
		token_type get_token_generator_additional_token() {
			if (this->is_additional_token) {
				return this->generator->translate_string_through_names_stack(*this->detached_name);
			}
			else {
				return this->generator->get_additional_token();
			}
		}
		bool is_token_generator_name_empty() {
			if (this->detached_name) {
				return this->detached_name->empty();
			}

			return this->generator->is_name_empty();
		}

		void switch_context(context_key_type key) {
			this->generator->set_current_context(key);
		}

		bool is_working() {
			return this->working && (this->states_stack.size() != 0);
		}
		friend class states_builder<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class read_map : public read_map_base<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type> {
	private:
		using base = read_map_base<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;

	public:
		read_map(token_type end_token, token_type name_token, token_generator<token_type, context_key_type>* token_generator)
			:base{ end_token, name_token, token_generator }
		{
		}

		void handle_token(objects* builder_object, token_type token, parameters_object* parameters) {
			typename base::state_type* top_state = this->states_stack.top();
			if (this->initialize_handle(token, top_state)) {
				this->call_handler(top_state, builder_object, parameters, this);

				if (!this->handle_error_exit) { //handles can use exit_with_error to set an error inside handle_token
					this->redirect_to_another_state(top_state, &this->get_parameters_container(), parameters);
					this->check_state_behaviour(top_state);
				}
			}
		}
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_enumeration_type>
	class read_map<token_type, context_key_type, objects, void, parameters_enumeration_type> : public read_map_base<token_type, context_key_type, objects, void, parameters_enumeration_type> {
	private:
		using base = read_map_base<token_type, context_key_type, objects, void, parameters_enumeration_type>;

	public:
		read_map(token_type end_token, token_type name_token, token_generator<token_type, context_key_type>* token_generator)
			:base{ end_token, name_token, token_generator }
		{
		}

		void handle_token(objects* builder_object, token_type token) {
			typename base::state_type* top_state = this->states_stack.top();
			if (this->initialize_handle(token, top_state)) {
				this->call_handler(top_state, builder_object, this);

				if (!this->handle_error_exit) { //handles can use exit_with_error to set an error inside handle_token
					this->redirect_to_another_state(top_state, &this->get_parameters_container());
					this->check_state_behaviour(top_state);
				}
			}
		}
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class states_builder {
	private:
		template<typename parameters_object>
		struct get_function_signature {
			using value = void(objects&, parameters_object&, read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>&);
		};

		template<>
		struct get_function_signature<void> {
			using value = void(objects&, read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>&);
		};

	public:
		using state_type =
			state<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;

		using state_settings_type =
			state_settings<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;

		using custom_function_signature =
			typename state_type::custom_function_signature;

		using anonymous_function_signature = typename get_function_signature<parameters_object>::value;

	private:
		size_t starting_state_index{};

		std::vector<std::unique_ptr<state_type>> states;
		std::forward_list<state_settings_type> states_settings;

		template<typename parameters_object, typename function_signature>
		class anonymous_state : public state_type {
		private:
			std::function<function_signature> lambda_function;

		public:
			anonymous_state(std::function<function_signature>&& custom_function)
				:lambda_function{ std::move(custom_function) }
			{
			}

			virtual void handle_token(
				objects& file_objects,
				parameters_object& params,
				read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>& read_map
			) override {
				this->lambda_function(file_objects, params, read_map);
			}
		};

		template<typename function_signature>
		class anonymous_state<void, function_signature> : public state_type {
		private:
			std::function<function_signature> lambda_function;

		public:
			anonymous_state(std::function<function_signature>&& custom_function)
				:lambda_function{ std::move(custom_function) }
			{
			}

			virtual void handle_token(
				objects& file_objects,
				read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>& read_map
			) override {
				this->lambda_function(file_objects, read_map);
			}
		};

	public:
		template<typename T, typename... types>
		state_settings_type& create_state(types&&... values) {
			this->states_settings.emplace_front();
			this->states.push_back(
				std::make_unique<T>(std::forward<types>(values)...)
			);

			this->states_settings.front().settings_index = this->states.size() - 1;
			this->states_settings.front().states_builder_back_reference = this;
			this->states_settings.front().associated_state = this->states.back().get();

			return this->states_settings.front();
		}

		state_settings_type& create_anonymous_state(
			std::function<anonymous_function_signature> handle_token_implementation
		) {
			return this->create_state<anonymous_state<parameters_object, anonymous_function_signature>>(
				std::move(handle_token_implementation)
			);
		}

		void attach_states(
			read_map<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>* read_map
		) {
			read_map->states = std::move(this->states);
			read_map->states_stack.push(
				read_map->states[this->starting_state_index].get()
			);
		}

		friend class state_settings_type;
	};

	template<typename token_type, typename context_key_type, typename objects, typename parameters_object, typename parameters_enumeration_type>
	class state_settings {
	private:
		using states_builder_type =
			states_builder<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;

		using state_type = typename states_builder_type::state_type;
		using custom_function_signature = typename states_builder_type::custom_function_signature;

		using redirection_information_type =
			std::pair<state_type*, state_action>;

		size_t settings_index{};
		states_builder_type* states_builder_back_reference{};

		state_type* associated_state{};

		state_type* get_state_from_state_settings(const state_settings& other) {
			return this->states_builder_back_reference->states[other.settings_index].get();
		}
		state_type* get_destination_state(const state_settings* other, [[maybe_unused]] state_action action) {
			state_type* destination_state = nullptr;
			if (other != nullptr) {
				destination_state = this->get_state_from_state_settings(*other);
			}

			assert(
				!((action != state_action::pop_top) ^ (other != nullptr)) &&
				"You must use nullptr with 'pop_top' and you must provide a valid state for other state_actions"
			);

			return destination_state;
		}

		void inner_add_custom_function(
			std::function<custom_function_signature>&& function,
			state_action action,
			state_type* state
		) {
			this->associated_state->custom_functions.push_back(
				std::make_pair(
					std::move(function),
					redirection_information_type{ state, action }
				)
			);
		}
		void inner_set_redirection_for_token(
			token_type token,
			state_action action,
			state_type* state
		) {
			this->associated_state->redirections_for_tokens[token] =
				redirection_information_type{ state, action };
		}

		template<typename T>
		void add_redirection_for_every_token(
			const std::vector<token_type>& tokens,
			state_action action,
			T other
		) {
			for (token_type token : tokens) {
				this->set_redirection_for_token(token, action, other);
			}
		}

		bool check_detached_name_and_whitelist_consistency() {
			if (this->associated_state->detached_name) {
				return this->associated_state->whitelist;
			}

			return true;
		}
	public:
		state_settings& set_as_starting_state() {
			this->states_builder_back_reference->starting_state_index = this->settings_index;
			return *this;
		}

		state_settings& whitelist(bool value) {
			this->associated_state->whitelist = value;
			assert(this->check_detached_name_and_whitelist_consistency() && "You can use detached name only with whitelist states.");

			return *this;
		}

		state_settings& all_tokens_handler(bool value) {
			this->associated_state->all_tokens_handler = value;
			return *this;
		}

		state_settings& set_handle_tokens(
			std::vector<token_type>&& handle_tokens
		) {
			this->associated_state->handle_tokens = std::move(handle_tokens);
			return *this;
		}

		state_settings& add_handle_token(
			token_type token
		) {
			this->associated_state->handle_tokens.push_back(token);
			return *this;
		}

		state_settings& set_redirection_for_token(
			token_type token,
			state_action action,
			const state_settings* other
		) {
			this->inner_set_redirection_for_token(
				token,
				action,
				this->get_destination_state(other, action)
			);

			return *this;
		}

		state_settings& set_redirection_for_token(
			token_type token,
			state_action action,
			const state_settings& other
		) {
			assert(action != state_action::pop_top && "Use nullptr with state_action::pop_top");
			this->inner_set_redirection_for_token(
				token,
				action,
				this->get_state_from_state_settings(other)
			);

			return *this;
		}

		state_settings& set_redirection_for_tokens(
			const std::vector<token_type>& tokens,
			state_action action,
			const state_settings* other
		) {
			this->add_redirection_for_every_token<const state_settings*>(tokens, action, other);
			return *this;
		}

		state_settings& set_redirection_for_tokens(
			const std::vector<token_type>& tokens,
			state_action action,
			const state_settings& other
		) {
			this->add_redirection_for_every_token<const state_settings&>(tokens, action, other);
			return *this;
		}

		state_settings& add_custom_function(
			std::function<custom_function_signature> function,
			state_action action,
			const state_settings* other
		) {
			this->inner_add_custom_function(
				std::move(function),
				action,
				this->get_destination_state(other, action)
			);

			return *this;
		}

		state_settings& add_custom_function(
			std::function<custom_function_signature> function,
			state_action action,
			const state_settings& other
		) {
			assert(action != state_action::pop_top && "Use nullptr with state_action::pop_top");
			this->inner_add_custom_function(
				std::move(function),
				action,
				this->get_state_from_state_settings(other)
			);
		}

		state_settings& set_error_message(
			std::string&& message
		) {
			this->associated_state->error_message = std::move(message);
			return *this;
		}

		state_settings& detached_name(bool value) {
			this->associated_state->detached_name = value;
			assert(this->check_detached_name_and_whitelist_consistency() && "You can use detached name only with whitelist states.");

			return *this;
		}

		friend class states_builder<token_type, context_key_type, objects, parameters_object, parameters_enumeration_type>;
	};
}

#endif // !GENERIC_PARSER_BUILDER_STATE_H