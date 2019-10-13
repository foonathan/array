// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_SMALL_FLAT_SET_HPP_INCLUDED
#define FOONATHAN_ARRAY_SMALL_FLAT_SET_HPP_INCLUDED

#include <foonathan/array/block_storage_default.hpp>
#include <foonathan/array/block_storage_heap_sbo.hpp>
#include <foonathan/array/flat_set.hpp>

namespace foonathan
{
namespace array
{
    /// Convenience alias for [array::flat_set<Key>]() with a small size optimization.
    template <typename Key, std::size_t SmallN, class Compare = key_compare_default,
              class Heap = default_heap, class Growth = default_growth>
    using small_flat_set
        = flat_set<Key, Compare, block_storage_heap_sbo<SmallN * sizeof(Key), Heap, Growth>>;

    /// Convenience alias for [array::flat_multiset<Key>]() with a small size optimization.
    template <typename Key, std::size_t SmallN, class Compare = key_compare_default,
              class Heap = default_heap, class Growth = default_growth>
    using small_flat_multiset
        = flat_multiset<Key, Compare, block_storage_heap_sbo<SmallN * sizeof(Key), Heap, Growth>>;
} // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_SMALL_FLAT_SET_HPP_INCLUDED
