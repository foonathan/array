// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/byte_view.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("byte_view")
{
    std::uint8_t array[] = {0, 1, 255};

    auto bytes = byte_view(make_array_view(array));
    REQUIRE((static_cast<void*>(bytes.data()) == array));
    REQUIRE(bytes.size() == 3u);
    REQUIRE(bytes[0] == 0);
    REQUIRE(bytes[1] == 1);
    REQUIRE(bytes[2] == 255);

    auto array_view = reinterpret_array<std::int8_t>(bytes);
    REQUIRE((static_cast<void*>(array_view.data()) == array));
    REQUIRE(array_view.size() == 3u);
    REQUIRE(array_view[0] == 0);
    REQUIRE(array_view[1] == 1);
    REQUIRE(array_view[2] == -1);

    auto block_view = reinterpret_block<std::int8_t>(bytes);
    REQUIRE((static_cast<void*>(block_view.data()) == array));
    REQUIRE(block_view.size() == 3u);
}
