#ifndef COMPRESSION_ALGORITHMS_STATIC_HUFFMAN_H
#define COMPRESSION_ALGORITHMS_STATIC_HUFFMAN_H

#include <vector>
#include <type_traits>
#include <map>
#include <algorithm>
#include <ranges>
#include <cassert>
#include <memory>
#include <limits>
#include <queue>
#include <unordered_map>
#include <stdexcept>

namespace compression_algorithms {
    /// <summary>
    /// Implementation of static Huffman encoding and decoding algorithm.
    /// </summary>
    /// <typeparam name="size_type">Type used for size and frequency values.</typeparam>
    /// <typeparam name="char_type">Type used for individual characters/symbols.</typeparam>
    template<typename size_type, typename char_type>
    class static_huffman {
        /// <summary>
        /// Structure representing a node in the Huffman encoding tree.
        /// </summary>
        struct encode_object {
            /// <summary>Frequency count of the symbol.</summary>
            size_type count;
            /// <summary>Pointer to the parent node.</summary>
            encode_object* upper_object;
            /// <summary>Symbol value.</summary>
            char_type symbol;
            /// <summary>A bit value indicating the branch (0 for left, 1 for right).</summary>
            bool bit;
        };

        /// <summary>
        /// Structure representing a node in the Huffman decoding tree.
        /// </summary>
        struct decode_object {
            /// <summary>Frequency count of the symbol.</summary>
            size_type count;
            /// <summary>Pointer to the left child node.</summary>
            decode_object* left;
            /// <summary>Pointer to the right child node.</summary>
            decode_object* right;
            /// <summary>Symbol value.</summary>
            char_type symbol;
        };

        /// <summary>
        /// Comparator function for ordering nodes deterministically.
        /// </summary>
        /// <param name="left">First node to compare.</param>
        /// <param name="right">Second node to compare.</param>
        /// <returns>True if left should come before right in the ordering.</returns>
        static bool deterministic_comparator(auto left, auto right) {
            // Additional ordering logic to ensure deterministic behavior
            if (left->count == right->count) {
                return left->symbol < right->symbol;
            }

            return left->count > right->count;
        }

        /// <summary>
        /// Helper class for managing multiple dynamically allocated objects.
        /// Ensures proper cleanup of all objects at destruction time.
        /// </summary>
        class multi_unique_pointer {
            /// <summary>Collection of pointers to managed objects.</summary>
            std::vector<void*> objects{};

        public:
            /// <summary>
            /// Constructs multi_unique_pointer without any objects attached.
            /// </summary>
            multi_unique_pointer() = default;

            /// <summary>
            /// Adds a pointer to be managed by this object.
            /// </summary>
            /// <typeparam name="T">Type of the object being managed.</typeparam>
            /// <param name="pointer">Unique pointer to take ownership from.</param>
            template<typename T>
            void add(std::unique_ptr<T>& pointer) {
                static_assert(std::is_trivially_destructible_v<T>, "Type is not trivially destructible.");
                this->objects.push_back(pointer.get());
                pointer.release();
            }

            multi_unique_pointer(const multi_unique_pointer&) = delete;
            multi_unique_pointer(multi_unique_pointer&&) = delete;

            multi_unique_pointer& operator=(const multi_unique_pointer&) = delete;
            multi_unique_pointer& operator=(multi_unique_pointer&&) = delete;

            /// <summary>
            /// Destructor that deallocates all managed objects.
            /// </summary>
            ~multi_unique_pointer() noexcept {
                for (auto& object : this->objects) {
                    ::operator delete(object);
                }
            }
        };

        /// <summary>Collection of nodes for decoding.</summary>
        std::vector<std::unique_ptr<decode_object>> decode_table{};
        
        /// <summary>Collection of nodes for encoding.</summary>
        std::vector<std::unique_ptr<encode_object>> encode_table{};

    public:
        /// <summary>
        /// Creates an encoding table from an input sequence by counting symbol frequencies.
        /// </summary>
        /// <typeparam name="input_iter">Iterator type for input sequence.</typeparam>
        /// <param name="start">Iterator to the beginning of the input sequence.</param>
        /// <param name="end">Iterator to the end of the input sequence.</param>
        template<typename input_iter>
        void create_encode_table(input_iter start, input_iter end) {
            static_assert(std::is_same_v<typename std::iterator_traits<input_iter>::value_type, char_type>, "Different char types.");

            std::unordered_map<char_type, size_type> frequency_map;
            for (; start != end; ++start) {
                char_type symbol = *start;
                ++frequency_map[symbol];
            }

            for (const auto& [symbol, count] : frequency_map) {
                this->encode_table.push_back(std::make_unique<encode_object>(encode_object{
                    .count = count,
                    .upper_object = nullptr,
                    .symbol = symbol,
                    .bit = false
                }));
            }
        }

        /// <summary>
        /// Copies all entries from the encode table to the decode table.
        /// </summary>
        void copy_encode_table_to_decode_table() {
            for (const auto& obj : this->encode_table) {
                this->decode_table.push_back(std::make_unique<decode_object>(decode_object{
                    .count = obj->count,
                    .left = nullptr,
                    .right = nullptr,
                    .symbol = obj->symbol
                    }));
            }
        }

        /// <summary>
        /// Copies all entries from the decode table to the encode table.
        /// </summary>
        void copy_decode_table_to_encode_table() {
            for (const auto& obj : this->decode_table) {
                this->encode_table.push_back(std::make_unique<encode_object>(encode_object{
                    .count = obj->count,
                    .upper_object = nullptr,
                    .symbol = obj->symbol,
                    .bit = false
                    }));
            }
        }

        /// <summary>
        /// Adds a new decode object to the decode table at the specified index.
        /// </summary>
        /// <param name="count">Frequency count for the symbol.</param>
        /// <param name="symbol">Character/symbol value.</param>
        /// <param name="index">Position where to insert the new object.</param>
        void add_decode_object(size_type count, char_type symbol, std::size_t index) {
            this->decode_table.insert(this->decode_table.begin() + static_cast<std::ptrdiff_t>(index),
                std::make_unique<decode_object>(decode_object{
                    .count = count,
                    .left = nullptr,
                    .right = nullptr,
                    .symbol = symbol
                    }));
        }

        /// <summary>
        /// Adds a new encode object to the encode table at the specified index.
        /// </summary>
        /// <param name="count">Frequency count for the symbol.</param>
        /// <param name="symbol">Character/symbol value.</param>
        /// <param name="index">Position where to insert the new object.</param>
        void add_encode_object(size_type count, char_type symbol, std::size_t index) {
            this->encode_table.insert(this->encode_table.begin() + index, 
                std::make_unique<encode_object>(encode_object{
                    .count = count,
                    .upper_object = nullptr,
                    .symbol = symbol,
                    .bit = false
                }));
        }

        /// <summary>
        /// Removes a decode object at the specified index.
        /// </summary>
        /// <param name="index">Index of the object to remove.</param>
        void erase_decode_object(std::size_t index) {
            this->decode_table.erase(this->decode_table.begin() + index);
        }

        /// <summary>
        /// Gets the number of entries in the encode table.
        /// </summary>
        /// <returns>Size of the encode table.</returns>
        std::size_t get_encode_table_size() const noexcept {
            return this->encode_table.size();
        }

        /// <summary>
        /// Gets the number of entries in the decode table.
        /// </summary>
        /// <returns>Size of the decode table.</returns>
        std::size_t get_decode_table_size() const noexcept {
            return this->decode_table.size();
        }

        /// <summary>
        /// Removes an encode object at the specified index.
        /// </summary>
        /// <param name="index">Index of the object to remove.</param>
        void erase_encode_object(std::size_t index) {
            this->encode_table.erase(this->encode_table.begin() + index);
        }

        /// <summary>
        /// Gets a reference to the count of an encode object.
        /// </summary>
        /// <param name="index">Index of the encode object.</param>
        /// <returns>Reference to the count value.</returns>
        size_type& get_encode_object_count(std::size_t index) noexcept {
            return this->encode_table[index]->count;
        }

        /// <summary>
        /// Gets a reference to the count of a decode object.
        /// </summary>
        /// <param name="index">Index of the decode object.</param>
        /// <returns>Reference to the count value.</returns>
        size_type& get_decode_object_count(std::size_t index) noexcept {
            return this->decode_table[index]->count;
        }

        /// <summary>
        /// Gets a reference to the symbol of an encode object.
        /// </summary>
        /// <param name="index">Index of the encode object.</param>
        /// <returns>Reference to the symbol value.</returns>
        char_type& get_encode_object_symbol(std::size_t index) noexcept {
            return this->encode_table[index]->symbol;
        }

        /// <summary>
        /// Gets a reference to the symbol of a decode object.
        /// </summary>
        /// <param name="index">Index of the decode object.</param>
        /// <returns>Reference to the symbol value.</returns>
        char_type& get_decode_object_symbol(std::size_t index) noexcept {
            return this->decode_table[index]->symbol;
        }

        /// <summary>
        /// Decodes a Huffman-encoded message using the decode table.
        /// </summary>
        /// <typeparam name="input_iterator">Iterator type for input sequence.</typeparam>
        /// <typeparam name="output_iterator">Iterator type for output sequence.</typeparam>
        /// <param name="start">Iterator to the beginning of the encoded message.</param>
        /// <param name="end">Used to ensure that no buffer overflow can happen.</param>
        /// <param name="message_size">Number of symbols in the original message.</param>
        /// <param name="out">Output iterator for writing the decoded message.</param>
        template<typename input_iterator, typename output_iterator>
        void decode(input_iterator start, input_iterator end, size_type message_size, output_iterator out) {
            static_assert(std::is_same_v<typename std::iterator_traits<input_iterator>::value_type, char_type>, "Different char types.");
            if (this->decode_table.empty() || message_size == 0) {
                return;
            }

            // Controls memory deallocation for when exceptions occur
            multi_unique_pointer memory_control{};
            std::priority_queue<
                decode_object*,
                std::vector<decode_object*>,
                bool(*)(decode_object*, decode_object*)
            > queue(&static_huffman::deterministic_comparator);

            for (const auto& obj : this->decode_table) {
                queue.push(obj.get());
            }

            // Build Huffman tree using priority queue
            while (queue.size() > 1) {
                decode_object* first = queue.top(); queue.pop();
                decode_object* second = queue.top(); queue.pop();

                std::unique_ptr<decode_object> new_object{
                    new decode_object{
                        .count = first->count + second->count,
                        .left = second,   // larger count goes to left
                        .right = first,   // lesser count goes to right
                        .symbol = std::max(first->symbol, second->symbol)
                    }
                };

                queue.push(new_object.get());
                memory_control.add(new_object); // For proper memory cleanup
            }

            constexpr char_type first_bit = char_type{ 1 } << (std::numeric_limits<char_type>::digits - 1);
            constexpr std::size_t bits_count = std::numeric_limits<char_type>::digits;

            decode_object* root = queue.top(); // The root is the only element left in the queue
            std::size_t bits_written = 0;

            char_type symbol = *start;
            ++start;

            for (size_type count = 0; count != message_size;) {
                if (first_bit >> bits_written & symbol) { // If bit is 1 - right, if 0 - left
                    root = root->right;
                }
                else {
                    root = root->left;
                }
                ++bits_written;

                if (!root->left) { // Checking if it's a leaf node
                    assert(!root->left && !root->right); // Must be a leaf. Assuming that tree nodes are either inner nodes or leaves. If left is null, right must also be null
                    *out = root->symbol;
                    ++out;

                    root = queue.top(); // Start from root again
                    ++count;
                }

                if (bits_written == bits_count && count != message_size) { // If all bits are read
                    if (start == end) {
                        throw std::runtime_error{ "Static Huffman: Unexpected end of input." };
                    }

                    symbol = *start;
                    ++start;

                    bits_written = 0;
                }
            }
        }

        /// <summary>
        /// Encodes a message using Huffman encoding based on the encode table.
        /// </summary>
        /// <typeparam name="input_iterator">Iterator type for input sequence.</typeparam>
        /// <typeparam name="output_iterator">Iterator type for output sequence.</typeparam>
        /// <param name="start">Iterator to the beginning of the message to encode.</param>
        /// <param name="end">Iterator to the end of the message to encode.</param>
        /// <param name="out">Output iterator for writing the encoded message.</param>
        template<typename input_iterator, typename output_iterator>
        void encode(input_iterator start, input_iterator end, output_iterator out) {
            static_assert(std::is_same_v<typename std::iterator_traits<input_iterator>::value_type, char_type>, "Different char types.");
            if (this->encode_table.empty() || start == end) { // If the table has entries and there's data to encode
                return;
            }

            multi_unique_pointer memory_control{};
            std::priority_queue<
                encode_object*,
                std::vector<encode_object*>,
                bool(*)(encode_object*, encode_object*)
            > queue(&static_huffman::deterministic_comparator);

            for (const auto& obj : this->encode_table) {
                queue.push(obj.get());
            }

            // Build Huffman tree using priority queue
            while (queue.size() > 1) {
                encode_object* first = queue.top(); queue.pop();
                encode_object* second = queue.top(); queue.pop();

                std::unique_ptr<encode_object> new_object{
                    new encode_object{
                        .count = first->count + second->count,
                        .upper_object = nullptr,
                        .symbol = std::max(first->symbol, second->symbol),
                        .bit = false
                    }
                };

                // Set parent relationships
                first->upper_object = new_object.get();
                second->upper_object = new_object.get();
                
                // Set bit values to identify branches
                first->bit = true;    // lesser count goes to right
                second->bit = false;  // larger count goes to left

                queue.push(new_object.get());
                memory_control.add(new_object);
            }

            std::map<char_type, std::vector<encode_object*>> char_chains{}; // Map with chains of objects for each character
            for (auto begin = this->encode_table.begin(), l_end = this->encode_table.end(); begin != l_end; ++begin) {
                encode_object* found = begin->get();
                char_type symbol = found->symbol;
                do { // Using do because the leaf itself might be the only element in tree
                    char_chains[symbol].push_back(found);
                    found = found->upper_object;
                } while (found && found->upper_object);
            }

            constexpr char_type first_bit = char_type{ 1 } << (std::numeric_limits<char_type>::digits - 1);
            constexpr std::size_t bits_count = std::numeric_limits<char_type>::digits; // Number of bits in the character type
            std::size_t bits_written = 0;
            char_type to_write{};
            for (; start != end; ++start) {
                char_type symbol = *start;
                std::vector<encode_object*>& found = char_chains[symbol];

                // Process in reverse order (from root to leaf)
                for (auto reversed_begin = found.rbegin(), reversed_end = found.rend(); reversed_begin != reversed_end; ++reversed_begin) {
                    if (bits_written == bits_count) { // If we've filled a byte
                        *out = to_write;
                        ++out;

                        to_write = char_type{};
                        bits_written = 0;
                    }

                    if ((*reversed_begin)->bit) {
                        to_write |= first_bit >> bits_written;
                    }
                    ++bits_written;
                }
            }

            if (bits_written) {
                *out = to_write;
                ++out;
            }
        }
    };
}

#endif
