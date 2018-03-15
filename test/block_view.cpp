// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/block_view.hpp>

#include <catch.hpp>

using namespace foonathan::array;

namespace
{
    template <typename T>
    void test_block_view(const block_view<T>& view, const memory_block& expected)
    {
        REQUIRE(view.empty() == expected.empty());
        REQUIRE(view.size() == expected.size() / sizeof(T));

        REQUIRE(view.data() == to_pointer<T>(expected.begin()));
        REQUIRE(view.data_end() == to_pointer<T>(expected.end()));

        REQUIRE(view.begin().operator->() == view.data());
        REQUIRE(view.end().operator->() == view.data_end());

        REQUIRE(view.block().begin() == expected.begin());
        REQUIRE(view.block().end() == expected.end());
    }
}

TEST_CASE("block_view", "[view]")
{
    int  array[] = {1, 2, 3};
    auto block   = memory_block(as_raw_pointer(array), 3 * sizeof(int));

    SECTION("empty")
    {
        block_view<int> view;
        test_block_view(view, memory_block());
    }
    SECTION("memory_block")
    {
        block_view<int> view(block);
        test_block_view(view, block);
    }
    SECTION("data + size")
    {
        block_view<int> view(array, 3);
        test_block_view(view, block);
        view = make_block_view(array, 3);
        test_block_view(view, block);
    }
    SECTION("begin + end")
    {
        block_view<int> view(array, array + 3);
        test_block_view(view, block);
        view = make_block_view(array, array + 3);
        test_block_view(view, block);
    }
    SECTION("array")
    {
        block_view<int> view(array);
        test_block_view(view, block);
        view = make_block_view(array);
        test_block_view(view, block);
    }
    SECTION("initializer_list")
    {
        block_view<const int> view = {1, 2, 3};
        // can't use test_block_view as I don't know where it points to
        REQUIRE(view.size() == 3u);
        REQUIRE(view.data()[1] == 2);
    }
}
