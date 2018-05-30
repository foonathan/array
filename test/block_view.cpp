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
    template <typename T>
    void test_block_view(const block_view<const T>& view, const memory_block& expected)
    {
        REQUIRE(view.empty() == expected.empty());
        REQUIRE(view.size() == expected.size() / sizeof(T));

        REQUIRE(view.data() == to_pointer<T>(expected.begin()));
        REQUIRE(view.data_end() == to_pointer<T>(expected.end()));

        REQUIRE(view.begin().operator->() == view.data());
        REQUIRE(view.end().operator->() == view.data_end());
    }
} // namespace

TEST_CASE("block_view", "[view]")
{
    int  array[] = {1, 2, 3};
    auto block   = memory_block(to_raw_pointer(array), 3 * sizeof(int));

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
        REQUIRE((std::is_same<block_value_type<decltype(array)>, int>::value));
        REQUIRE((std::is_same<block_value_type<const int[4]>, const int>::value));

        block_view<int> view(array);
        test_block_view(view, block);
        view = make_block_view(array);
        test_block_view(view, block);
    }
    SECTION("initializer_list")
    {
        REQUIRE((std::is_same<block_value_type<std::initializer_list<int>>, const int>::value));

        block_view<const int> view = {1, 2, 3};
        // can't use test_block_view as I don't know where it points to
        REQUIRE(view.size() == 3u);
        REQUIRE(view.data()[1] == 2);
    }
    SECTION("custom")
    {
        struct custom_block
        {
            using value_type = int;

            operator block_view<int>()
            {
                return block_view<int>(memory_block());
            }
            operator block_view<const int>() const
            {
                return block_view<const int>(memory_block());
            }
        };

        custom_block       block;
        const custom_block cblock{};

        REQUIRE((std::is_same<block_value_type<custom_block>, int>::value));
        block_view<int> view(block);
        test_block_view(view, memory_block());

        REQUIRE((std::is_same<block_value_type<const custom_block>, const int>::value));
        block_view<const int> cview(block);
        test_block_view(cview, memory_block());

        cview = block_view<const int>(cblock);
        test_block_view(cview, memory_block());
    }
}
