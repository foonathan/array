// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/view.hpp>

#include <catch.hpp>

using namespace foonathan::array;

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

TEST_CASE("array_view")
{
    // no need to test much, most is already handled by block_view
    const int carray[] = {1, 2, 3};
    int       array[]  = {1, 2, 3};

    SECTION("creation")
    {
        array_view<const int> cview;
        SECTION("non-const")
        {
            cview = make_array_view(array);
        }
        SECTION("const")
        {
            cview = make_array_view(carray);
        }

        REQUIRE(cview[0] == 1);
        REQUIRE(cview[1] == 2);
        REQUIRE(cview[2] == 3);
        REQUIRE(cview.front() == 1);
        REQUIRE(cview.back() == 3);
    }

    array_view<int> view = make_array_view(array);
    view[1]              = 42;
    REQUIRE(array[1] == 42);

    SECTION("slice")
    {
        array_view<int> slice_view = view.slice(1u, 1u);
        REQUIRE(slice_view.size() == 1u);
        REQUIRE(slice_view.data() == array + 1);

        slice_view = view.slice(view.begin() + 1, 1u);
        REQUIRE(slice_view.size() == 1u);
        REQUIRE(slice_view.data() == array + 1);
    }
}

TEST_CASE("byte_view")
{
    std::uint8_t array[] = {0, 1, 255};

    auto bytes = byte_view(make_array_view(array));
    REQUIRE(static_cast<void*>(bytes.data()) == array);
    REQUIRE(bytes.size() == 3u);
    REQUIRE(bytes[0] == 0);
    REQUIRE(bytes[1] == 1);
    REQUIRE(bytes[2] == 255);

    auto array_view = reinterpret_array<std::int8_t>(bytes);
    REQUIRE(static_cast<void*>(array_view.data()) == array);
    REQUIRE(array_view.size() == 3u);
    REQUIRE(array_view[0] == 0);
    REQUIRE(array_view[1] == 1);
    REQUIRE(array_view[2] == -1);

    auto block_view = reinterpret_block<std::int8_t>(bytes);
    REQUIRE(static_cast<void*>(block_view.data()) == array);
    REQUIRE(block_view.size() == 3u);
}
