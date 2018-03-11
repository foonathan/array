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

        /// A contiguous block of memory.
        struct memory_block
        {
            void*     memory;
            size_type size;

            constexpr memory_block(void* memory, size_type size) : memory(memory), size(size) {}
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_MEMORY_BLOCK_HPP_INCLUDED
