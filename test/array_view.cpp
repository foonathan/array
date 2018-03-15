// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/array_view.hpp>

#include <catch.hpp>

using namespace foonathan::array;

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
