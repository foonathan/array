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

        /// A pointer to a memory block.
        /// \notes This is used to provide byte-wise access and pointer arithmetic.
        using raw_pointer = unsigned char*;

        constexpr raw_pointer from_pointer(void* ptr) noexcept
        {
            return static_cast<unsigned char*>(ptr);
        }

        constexpr void* to_void_pointer(raw_pointer ptr) noexcept
        {
            return ptr;
        }

        template <typename T>
        constexpr T* to_pointer(raw_pointer ptr) noexcept
        {
            return static_cast<T*>(to_void_pointer(ptr));
        }

        /// A contiguous block of memory.
        struct memory_block
        {
            raw_pointer memory;
            size_type   size;

            /// \effects Creates an empty block.
            constexpr memory_block() noexcept : memory(nullptr), size(0) {}

            constexpr memory_block(raw_pointer memory, size_type size) noexcept
            : memory(memory), size(size)
            {
            }

            /// \returns The maximum size of a memory block.
            static constexpr size_type max_size() noexcept
            {
                return size_type(-1);
            }
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_MEMORY_BLOCK_HPP_INCLUDED
