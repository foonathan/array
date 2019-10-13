// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/small_array.hpp>
#include <foonathan/array/small_bag.hpp>
#include <foonathan/array/small_flat_set.hpp>

using namespace foonathan::array;

// just use the typedefs
using my_array    = small_array<int, 4>;
using my_bag      = small_bag<int, 2>;
using my_set      = small_flat_set<int, 2>;
using my_multiset = small_flat_multiset<int, 2>;
