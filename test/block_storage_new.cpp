// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/block_storage_new.hpp>

#include <catch.hpp>

#include "block_storage_algorithm.hpp"

using namespace foonathan::array;

TEST_CASE("block_storage_new", "[BlockStorage]")
{
    // TODO: proper test of storage itself

    using namespace test;
    test_clear_and_shrink<block_storage_new<default_growth>>({});
    test_clear_and_reserve<block_storage_new<default_growth>>({});
    test_move_to_front<block_storage_new<default_growth>>({});
    test_assign<block_storage_new<default_growth>>({});
    test_fill<block_storage_new<default_growth>>({});
    test_move_assign<block_storage_new<default_growth>>({});
    test_copy_assign<block_storage_new<default_growth>>({});
}
