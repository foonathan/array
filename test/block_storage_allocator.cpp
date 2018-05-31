// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/block_storage_allocator.hpp>

#include <catch.hpp>

#include "block_storage_algorithm.hpp"

using namespace foonathan::array;

namespace
{
    struct non_empty_allocator : std::allocator<byte>
    {
        int dummy;

        template <typename T>
        struct rebind
        {
            using other = non_empty_allocator;
        };

        explicit non_empty_allocator(int i) : dummy(i) {}
    };

} // namespace

TEST_CASE("block_storage_allocator", "[BlockStorage]")
{
    SECTION("empty")
    {
        REQUIRE(sizeof(block_storage_allocator<std::allocator<int>, default_growth>)
                == sizeof(memory_block));

        test::test_block_storage_algorithm<
            block_storage_allocator<std::allocator<int>, default_growth>>({});
        test::test_block_storage_algorithm<
            block_storage_allocator<std::allocator<int>, no_extra_growth>>({});
    }
    SECTION("non-empty")
    {
        test::test_block_storage_algorithm<
            block_storage_allocator<non_empty_allocator, default_growth>>(non_empty_allocator(42));
        test::test_block_storage_algorithm<
            block_storage_allocator<non_empty_allocator, no_extra_growth>>(non_empty_allocator(42));
    }
}
