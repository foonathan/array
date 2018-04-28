// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <catch.hpp>

#include <foonathan/array/key_compare.hpp>

#include <vector>

using namespace foonathan::array;

namespace
{
    template <typename Compare>
    struct less
    {
        template <typename T, typename U>
        bool operator()(const T& t, const U& u)
        {
            auto result = Compare::compare(t, u);
            return result == key_ordering::less;
        }
    };

    template <typename Compare>
    void test_impl(const std::vector<int>& container, int value, std::size_t index,
                   std::size_t count = 1)
    {
        INFO(value);

        auto lower =
            foonathan::array::lower_bound<Compare>(container.begin(), container.end(), value);
        REQUIRE(std::size_t(lower - container.begin()) == index);

        auto std_lower =
            std::lower_bound(container.begin(), container.end(), value, less<Compare>{});
        REQUIRE(std::size_t(std_lower - container.begin()) == index);

        auto upper =
            foonathan::array::upper_bound<Compare>(container.begin(), container.end(), value);
        REQUIRE(std::size_t(upper - container.begin()) == index + count);

        auto std_upper =
            std::upper_bound(container.begin(), container.end(), value, less<Compare>{});
        REQUIRE(std::size_t(std_upper - container.begin()) == index + count);

        auto range =
            foonathan::array::equal_range<Compare>(container.begin(), container.end(), value);
        REQUIRE(range.begin() == lower);
        REQUIRE(range.end() == upper);
    }
} // namespace

TEST_CASE("lower_bound/upper_bound/equal_range", "[util]")
{
    SECTION("key_compare_default")
    {
        using compare = key_compare_default<int>;

        std::vector<int> vec = {1, 2, 3, 5, 5, 5, 6, 7, 8};

        test_impl<compare>(vec, 1, 0);
        test_impl<compare>(vec, 2, 1);
        test_impl<compare>(vec, 3, 2);
        test_impl<compare>(vec, 5, 3, 3);
        test_impl<compare>(vec, 6, 6);
        test_impl<compare>(vec, 7, 7);
        test_impl<compare>(vec, 8, 8);

        test_impl<compare>(vec, 0, 0, 0); // less than all
        test_impl<compare>(vec, 9, 9, 0); // greater than all
        test_impl<compare>(vec, 4, 3, 0); // in between
    }
    SECTION("custom compare")
    {
        struct mod6_compare
        {
            static key_ordering compare(int lhs_, int rhs_)
            {
                auto lhs = lhs_ % 6;
                auto rhs = rhs_ % 6;

                if (lhs == rhs)
                    return key_ordering::equivalent;
                else if (lhs < rhs)
                    return key_ordering::less;
                else
                    return key_ordering::greater;
            }
        };

        std::vector<int> vec = {1, 7, 2, 3, 9, 4};

        test_impl<mod6_compare>(vec, 1, 0, 2);
        test_impl<mod6_compare>(vec, 7, 0, 2);
        test_impl<mod6_compare>(vec, 13, 0, 2);

        test_impl<mod6_compare>(vec, 2, 2);
        test_impl<mod6_compare>(vec, 8, 2);

        test_impl<mod6_compare>(vec, 3, 3, 2);
        test_impl<mod6_compare>(vec, 9, 3, 2);
        test_impl<mod6_compare>(vec, 15, 3, 2);

        test_impl<mod6_compare>(vec, 4, 5);
        test_impl<mod6_compare>(vec, 10, 5);

        test_impl<mod6_compare>(vec, 0, 0, 0);  // less than all
        test_impl<mod6_compare>(vec, 6, 0, 0);  // less than all
        test_impl<mod6_compare>(vec, 5, 6, 0);  // greater than all
        test_impl<mod6_compare>(vec, 11, 6, 0); // greater than all
    }
}
