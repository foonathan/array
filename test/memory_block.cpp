// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/memory_block.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("raw_pointer", "[core]")
{
    auto obj = 0;

    auto ptr = from_pointer(&obj);
    REQUIRE(to_void_pointer(ptr) == &obj);
    REQUIRE(to_pointer<int>(ptr) == &obj);

    REQUIRE(to_void_pointer(ptr + sizeof(obj)) == &obj + 1);
}

TEST_CASE("memory_block", "[core]")
{
    SECTION("empty")
    {
        memory_block block;
        REQUIRE(block.memory == nullptr);
        REQUIRE(block.size == 0u);
        REQUIRE(!block);
    }
    SECTION("non-empty")
    {
        auto obj = 0;

        memory_block block(from_pointer(&obj), sizeof(obj));
        REQUIRE(block.memory == from_pointer(&obj));
        REQUIRE(block.size == sizeof(obj));
        REQUIRE(!!block);
    }
}
