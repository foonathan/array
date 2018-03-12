// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/block_storage_arg.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("block_storage_args_t", "[core]")
{
    block_storage_args_t<int, const char*> a1 = block_storage_args(42, "Hello World!");
    REQUIRE(std::get<0>(a1.args) == 42);
    REQUIRE(std::get<1>(a1.args)[0] == 'H');

    block_storage_args_t<bool> a2 = block_storage_arg(true);
    REQUIRE(std::get<0>(a2.args));

    block_storage_args_t<> a3 = block_storage_args();
    (void)a3;
}
