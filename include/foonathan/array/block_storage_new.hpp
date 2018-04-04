// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED

#include <new>

#include <foonathan/array/block_storage_heap.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `Heap` that uses `::operator new`.
        struct new_heap
        {
            struct handle_type
            {
            };

            static memory_block allocate(handle_type&, size_type size, size_type)
            {
                auto ptr = ::operator new[](size);
                return {as_raw_pointer(ptr), size};
            }

            static void deallocate(handle_type&, memory_block&& block) noexcept
            {
                ::operator delete[](to_void_pointer(block.begin()));
            }
        };

        /// A `BlockStorage` that uses `operator new` for memory allocations.
        template <class GrowthPolicy>
        using block_storage_new = block_storage_heap<new_heap, GrowthPolicy>;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
