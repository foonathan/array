// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_DEFAULT_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_DEFAULT_HPP_INCLUDED

#include <foonathan/array/block_storage_new.hpp>

namespace foonathan
{
    namespace array
    {
        /// The default `Heap`.
        using default_heap = new_heap;

        /// The default `BlockStorage`.
        using block_storage_default = block_storage_new<>;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_DEFAULT_HPP_INCLUDED
