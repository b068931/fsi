#ifndef COMPRESSION_ALGORITHMS_SEQUENCE_REDUCTION_H
#define COMPRESSION_ALGORITHMS_SEQUENCE_REDUCTION_H

#include <vector>
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace compression_algorithms {
    /// <summary>
    /// Implements a compression algorithm that reduces repeated sequences in data.
    /// This algorithm searches for repeating patterns in the input and encodes them
    /// with references to earlier occurrences, reducing overall size.
    /// </summary>
    class sequence_reduction {
    private:
        /// <summary>
        /// Internal buffer used to store processed data during compression and decompression.
        /// </summary>
        std::vector<unsigned char> buffer{};

        /// <summary>
        /// Saves uncompressed data to the output stream with appropriate headers.
        /// </summary>
        /// <typeparam name="input_iterator">Type of the input iterator.</typeparam>
        /// <typeparam name="output_iterator">Type of the output iterator.</typeparam>
        /// <param name="buffer_iterator">Iterator pointing to the start of uncompressed data.</param>
        /// <param name="uncompressed_data_iterator">Iterator pointing to the end of uncompressed data.</param>
        /// <param name="out">Output iterator where the data will be written.</param>
        template<typename input_iterator, typename output_iterator>
        static void save_uncompressed_data(input_iterator buffer_iterator, const input_iterator& uncompressed_data_iterator, output_iterator& out) {
            unsigned short uncompressed_data_size = static_cast<unsigned short>(uncompressed_data_iterator - buffer_iterator);
            unsigned char* size_symbols = reinterpret_cast<unsigned char*>(&uncompressed_data_size);
            for (std::size_t count = 0; count < sizeof(unsigned short) * 2; ++count, ++out) {
                *out = static_cast<unsigned char>(0);
            }
            for (std::size_t count = 0; count < sizeof(unsigned short); ++count, ++size_symbols, ++out) {
                *out = *size_symbols;
            }

            for (; buffer_iterator != uncompressed_data_iterator; ++buffer_iterator, ++out) {
                *out = *buffer_iterator;
            }
        }

        /// <summary>
        /// Clears the internal buffer by setting all elements to null character.
        /// </summary>
        void clean() {
            for (unsigned char& beg : this->buffer) {
                beg = static_cast<unsigned char>(0);
            }
        }

    public:
        /// <summary>
        /// Initializes a new instance of the sequence_reduction class with a specified buffer size.
        /// </summary>
        /// <param name="size">The size of the internal buffer.</param>
        sequence_reduction(unsigned short size)
            :buffer(size)
        {
        }

        /// <summary>
        /// Compresses data from input range to the output iterator.
        /// </summary>
        /// <typeparam name="input_iterator">Type of the input iterator.</typeparam>
        /// <typeparam name="output_iterator">Type of the output iterator.</typeparam>
        /// <param name="start">Iterator pointing to the beginning of the input range.</param>
        /// <param name="end">Iterator pointing to the end of the input range.</param>
        /// <param name="out">Output iterator where compressed data will be written.</param>
        /// <param name="inefficient_size">Minimum sequence length for compression to be applied.</param>
        /// <remarks>
        /// The algorithm searches for repeated sequences in the input data and replaces them with
        /// references to their previous occurrences. Sequences shorter than uneffective_size are
        /// stored without compression.
        /// </remarks>
        template<typename input_iterator, typename output_iterator>
        void encode(input_iterator start, input_iterator end, output_iterator out, std::ptrdiff_t inefficient_size = 4) {
            std::vector<unsigned char> working_place(this->buffer.size());

            auto true_buffer_begin = this->buffer.begin();
            auto true_buffer_end = this->buffer.end();

            auto buffer_iterator = true_buffer_begin;
            auto uncompressed_data_iterator = true_buffer_begin;

            auto working_place_begin = working_place.begin();
            auto working_place_iterator = working_place_begin;
            auto working_place_end = working_place.end();
            while (start != end || working_place_begin != working_place_iterator) {
                for (; working_place_iterator != working_place_end && start != end; ++working_place_iterator, ++start) {
                    *working_place_iterator = std::bit_cast<unsigned char>(*start);
                }

                auto working_place_true_end = working_place_iterator;
                while (true) {
                    if (working_place_iterator - working_place_begin <= inefficient_size) {
                        std::ptrdiff_t written_data = working_place_iterator - working_place_begin;
                        auto write = working_place_begin;
                        while (write != working_place_iterator) {
                            *uncompressed_data_iterator = *write;

                            ++uncompressed_data_iterator;
                            ++write;

                            if (uncompressed_data_iterator == true_buffer_end) {
                                this->save_uncompressed_data(buffer_iterator, uncompressed_data_iterator, out);

                                buffer_iterator = true_buffer_begin;
                                uncompressed_data_iterator = buffer_iterator;
                            }
                        }

                        for (auto copy = working_place_begin; working_place_iterator != working_place_true_end; ++working_place_iterator, ++copy) {
                            *copy = *working_place_iterator;
                        }
                        working_place_iterator -= written_data;

                        break;
                    }
                    else {
                        auto found = std::search(
                            true_buffer_begin, 
                            true_buffer_end, 
                            working_place_begin, 
                            working_place_iterator
                        );

                        if (found != true_buffer_end) {
                            if (buffer_iterator != uncompressed_data_iterator) {
                                this->save_uncompressed_data(buffer_iterator, uncompressed_data_iterator, out);
                                buffer_iterator = uncompressed_data_iterator;
                            }

                            std::ptrdiff_t written_data = working_place_iterator - working_place_begin;
                            unsigned short substring_size = static_cast<unsigned short>(working_place_iterator - working_place_begin);
                            unsigned short offset_from_the_start = static_cast<unsigned short>(found - true_buffer_begin);

                            unsigned char* substring_size_symbols = reinterpret_cast<unsigned char*>(&substring_size);
                            unsigned char* offset_symbols = reinterpret_cast<unsigned char*>(&offset_from_the_start);
                            for (std::size_t count = 0; count < sizeof(unsigned short); ++count, ++substring_size_symbols, ++out) {
                                *out = *substring_size_symbols;
                            }
                            for (std::size_t count = 0; count < sizeof(unsigned short); ++count, ++offset_symbols, ++out) {
                                *out = *offset_symbols;
                            }

                            auto write = working_place_begin;
                            while (write != working_place_iterator) {
                                *buffer_iterator = *write;

                                ++write;
                                ++buffer_iterator;

                                if (buffer_iterator == true_buffer_end) {
                                    buffer_iterator = true_buffer_begin;
                                }
                            }
                            uncompressed_data_iterator = buffer_iterator;

                            for (auto copy = working_place_begin; working_place_iterator != working_place_true_end; ++working_place_iterator, ++copy) {
                                *copy = *working_place_iterator;
                            }
                            working_place_iterator -= written_data;

                            break;
                        }
                    }

                    --working_place_iterator;
                }
            }

            if (buffer_iterator != uncompressed_data_iterator) {
                this->save_uncompressed_data(buffer_iterator, uncompressed_data_iterator, out);
            }

            this->clean();
        }

        /// <summary>
        /// Decompresses data from input range to the output iterator.
        /// </summary>
        /// <typeparam name="input_iterator">Type of the input iterator.</typeparam>
        /// <typeparam name="output_iterator">Type of the output iterator.</typeparam>
        /// <param name="start">Iterator pointing to the beginning of the compressed data.</param>
        /// <param name="end">Iterator pointing to the end of the compressed data.</param>
        /// <param name="out">Output iterator where decompressed data will be written.</param>
        /// <remarks>
        /// Reconstructs the original data by interpreting compression markers and
        /// expanding the references to previously seen sequences.
        /// </remarks>
        template<typename input_iterator, typename output_iterator>
        void decode(input_iterator start, input_iterator end, output_iterator out) {
            std::vector<unsigned char> working_place(this->buffer.size());

            auto working_place_begin = working_place.begin();

            auto true_buffer_begin = this->buffer.begin();
            auto true_buffer_end = this->buffer.end();
            auto buffer_iterator = true_buffer_begin;
            while (start != end) {
                unsigned short first_pair = 0;
                unsigned short second_pair = 0;

                unsigned char* first_pair_symbols = reinterpret_cast<unsigned char*>(&first_pair);
                unsigned char* second_pair_symbols = reinterpret_cast<unsigned char*>(&second_pair);

                for (std::size_t count = 0; count < sizeof(unsigned short) && start != end; ++count, ++start, ++first_pair_symbols) {
                    *first_pair_symbols = std::bit_cast<unsigned char>(*start);
                }

                for (std::size_t count = 0; count < sizeof(unsigned short) && start != end; ++count, ++start, ++second_pair_symbols) {
                    *second_pair_symbols = std::bit_cast<unsigned char>(*start);
                }

                if (first_pair == 0 && second_pair == 0) {
                    unsigned short third_pair = 0;
                    unsigned char* third_pair_symbols = reinterpret_cast<unsigned char*>(&third_pair);
                    for (std::size_t count = 0; count < sizeof(unsigned short) && start != end; ++count, ++start, ++third_pair_symbols) {
                        *third_pair_symbols = std::bit_cast<unsigned char>(*start);
                    }

                    auto write = working_place_begin;
                    for (std::size_t count = 0; count < third_pair && start != end; ++count, ++write, ++start) {
                        *write = std::bit_cast<unsigned char>(*start);
                    }

                    for (auto work_begin = working_place_begin; work_begin != write; ++work_begin, ++out) {
                        unsigned char symbol = *work_begin;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

                        *out = symbol;

#pragma clang diagnostic pop

                        *buffer_iterator = symbol;
                        ++buffer_iterator;

                        if (buffer_iterator == true_buffer_end) {
                            buffer_iterator = true_buffer_begin;
                        }
                    }
                }
                else {
                    if (second_pair >= this->buffer.size() || first_pair > this->buffer.size() - second_pair) {
                        throw std::runtime_error("Sequence Reduction: buffer out of bounds.");
                    }

                    auto substring_begin = true_buffer_begin + second_pair;
                    auto write = working_place_begin;
                    for (std::size_t count = 0; count < first_pair; ++count, ++write, ++substring_begin) {
                        *write = *substring_begin;
                    }

                    for (auto work_begin = working_place_begin; work_begin != write; ++work_begin, ++out) {
                        unsigned char symbol = *work_begin;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

                        *out = symbol;

#pragma clang diagnostic pop

                        *buffer_iterator = symbol;
                        ++buffer_iterator;

                        if (buffer_iterator == true_buffer_end) {
                            buffer_iterator = true_buffer_begin;
                        }
                    }
                }
            }

            this->clean();
        }
    };
}

#endif
