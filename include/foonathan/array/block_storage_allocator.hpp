// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED

#include <memory>

#include <foonathan/array/block_storage_heap.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `Heap` that uses the given `Allocator` for allocation.
        ///
        /// The allocator will be rebound to [array::byte]().
        /// Only `allocate()` and `deallocate()` will be called, all other functions ignored.
        ///
        /// \note As the arguments of a `BlockStorage` will always propagate, so will the allocator,
        /// regardless of the `propagate_XXX` settings.
        template <class Allocator>
        struct allocator_heap
        {
            using handle_type =
                typename std::allocator_traits<Allocator>::template rebind_alloc<byte>;

            static memory_block allocate(handle_type& handle, size_type size, size_type)
            {
                auto ptr = std::allocator_traits<handle_type>::allocate(handle, size);
                return memory_block(ptr, size);
            }

            static void deallocate(handle_type& handle, memory_block&& block) noexcept
            {
                std::allocator_traits<handle_type>::deallocate(handle, block.begin(), block.size());
            }
        };

        /// A `BlockStorage` that uses an [array::allocator_heap]() for allocation.
        template <class Allocator, class GrowthPolicy>
        using block_storage_allocator = block_storage_heap<allocator_heap<Allocator>, GrowthPolicy>;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED
