// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/pointer_iterator.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("pointer_iterator", "[core]")
{
    struct tag
    {
    };
    using iter = pointer_iterator<tag, int>;

    REQUIRE(is_contiguous_iterator<iter>::value);

    SECTION("default")
    {
        iter def;
        REQUIRE(def.operator->() == nullptr);
        REQUIRE(iterator_to_pointer(def) == nullptr);
    }
    SECTION("non-default")
    {
        int array[] = {0, 1, 2, 3};

        auto begin = iter(tag{}, array);
        REQUIRE(begin.operator->() == array);
        REQUIRE(iterator_to_pointer(begin) == array);

        auto end = iter(tag{}, array + 4);
        REQUIRE(end.operator->() == array + 4);
        REQUIRE(iterator_to_pointer(end) == array + 4);

        SECTION("access")
        {
            REQUIRE(*begin == 0);
            REQUIRE(begin[2] == 2);
        }
        SECTION("increment/decrement")
        {
            ++begin;
            REQUIRE(begin.operator->() == array + 1);
            REQUIRE(begin++.operator->() == array + 1);
            REQUIRE(begin.operator->() == array + 2);

            --begin;
            REQUIRE(begin.operator->() == array + 1);
            REQUIRE(begin--.operator->() == array + 1);
            REQUIRE(begin.operator->() == array);
        }
        SECTION("+/-")
        {
            begin += 2;
            REQUIRE(begin.operator->() == array + 2);
            REQUIRE((begin - 1).operator->() == array + 1);

            begin -= 2;
            REQUIRE(begin.operator->() == array);
            REQUIRE((begin + 1).operator->() == array + 1);
        }
        SECTION("distance")
        {
            REQUIRE(end - begin == 4);
            REQUIRE(begin - end == -4);
        }
        SECTION("comparision")
        {
            REQUIRE(begin == begin);
            REQUIRE(begin != end);

            REQUIRE(begin < end);
            REQUIRE(begin <= begin);
            REQUIRE(begin <= end);

            REQUIRE(end > begin);
            REQUIRE(end >= begin);
            REQUIRE(end >= end);
        }
    }
}
