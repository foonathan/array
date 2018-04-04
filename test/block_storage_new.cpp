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
    REQUIRE(sizeof(block_storage_new<default_growth>) == sizeof(memory_block));

    test::test_block_storage_algorithm<block_storage_new<default_growth>>({});
    test::test_block_storage_algorithm<block_storage_new<no_extra_growth>>({});
}
