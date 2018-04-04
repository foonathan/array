// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_SBO_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_SBO_HPP_INCLUDED

#include <foonathan/array/block_storage_heap.hpp>
#include <foonathan/array/block_storage_sbo.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `BlockStorage` that has a small buffer of the given size it uses for small allocations,
        /// then uses the `Heap` for dynamic allocations growing with the specified `GrowthPolicy`.
        template <std::size_t SmallBufferBytes, class Heap, class GrowthPolicy>
        using block_storage_heap_sbo =
            block_storage_sbo<SmallBufferBytes, block_storage_heap<Heap, GrowthPolicy>>;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_SBO_HPP_INCLUDED
