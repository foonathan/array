// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_MEMORY_BLOCK_HPP_INCLUDED
#define FOONATHAN_ARRAY_MEMORY_BLOCK_HPP_INCLUDED

#include <cstddef>

namespace foonathan
{
    namespace array
    {
        /// The size of a memory block.
        using size_type = std::size_t;

#if FOONATHAN_ARRAY_HAS_BYTE
        using byte = std::byte;
#else
        using byte = unsigned char;
#endif

        /// A pointer to a memory block.
        /// \notes This is used to provide byte-wise access and pointer arithmetic.
        using raw_pointer = byte*;

        /// \returns The raw pointer to the same memory as `ptr`.
        constexpr raw_pointer to_raw_pointer(void* ptr) noexcept
        {
            return static_cast<byte*>(ptr);
        }

        /// \returns A `void*` to the same memory as `ptr`.
        constexpr void* to_void_pointer(raw_pointer ptr) noexcept
        {
            return ptr;
        }

        /// \returns A `T*` to the same memory as `ptr`.
        template <typename T>
        constexpr T* to_pointer(raw_pointer ptr) noexcept
        {
            return static_cast<T*>(to_void_pointer(ptr));
        }

        /// A contiguous block of memory.
        class memory_block
        {
        public:
            /// \returns The maximum size of a memory block.
            static constexpr size_type max_size() noexcept
            {
                return size_type(-1);
            }

            /// \effects Creates an empty block.
            constexpr memory_block() noexcept : begin_(nullptr), end_(nullptr) {}

            /// \effects Same as `memory_block(memory, memory + size)`.
            constexpr memory_block(raw_pointer memory, size_type size) noexcept
            : begin_(memory), end_(memory + size)
            {
            }

            /// \effects Creates a memory to the block delimited by the given range.
            constexpr memory_block(raw_pointer begin, raw_pointer end) noexcept
            : begin_(begin), end_(end)
            {
            }

            /// \returns Whether or not it is empty.
            constexpr bool empty() const noexcept
            {
                return begin_ == end_;
            }

            /// \returns The size of the memory block.
            constexpr size_type size() const noexcept
            {
                return size_type(end_ - begin_);
            }

            /// \returns A pointer to the beginning of the memory.
            constexpr raw_pointer begin() const noexcept
            {
                return begin_;
            }

            /// \returns A pointer to the end of the memory.
            constexpr raw_pointer end() const noexcept
            {
                return end_;
            }

            /// \returns A memory block with the same address but a different size.
            /// \notes This does not allocate memory or anything!
            constexpr memory_block resize(size_type new_size) const noexcept
            {
                return {begin(), new_size};
            }

        private:
            raw_pointer begin_;
            raw_pointer end_;
        };

        /// \returns A memory block to the given block of bytes.
        /// Use it with [std::aligned_storage_t](), for example.
        template <typename StaticBlock>
        constexpr memory_block static_memory_block(StaticBlock* block) noexcept
        {
            return memory_block(to_raw_pointer(block), to_raw_pointer(block) + sizeof(StaticBlock));
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_MEMORY_BLOCK_HPP_INCLUDED
