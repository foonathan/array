// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/growth_policy.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("no_extra_growth", "[GrowthPolicy]")
{
    REQUIRE(no_extra_growth::growth_size(0u, 4u, memory_block::max_size()) == 4u);
    REQUIRE(no_extra_growth::growth_size(4u, 8u, memory_block::max_size()) == 12u);

    REQUIRE(no_extra_growth::shrink_size(4u, 2u, memory_block::max_size()) == 2u);
    REQUIRE(no_extra_growth::shrink_size(8u, 8u, memory_block::max_size()) == 8u);
}

TEST_CASE("factor_growth", "[GrowthPolicy]")
{
    using factor1dot5 = factor_growth<3, 2>;

    REQUIRE(factor1dot5::growth_size(0u, 4u, memory_block::max_size()) == 4u);
    REQUIRE(factor1dot5::growth_size(4u, 1u, memory_block::max_size()) == 6u);
    REQUIRE(factor1dot5::growth_size(5u, 1u, memory_block::max_size()) == 7u);
    REQUIRE(factor1dot5::growth_size(4u, 8u, memory_block::max_size()) == 12u);

    REQUIRE(factor1dot5::shrink_size(4u, 2u) == 2u);
    REQUIRE(factor1dot5::shrink_size(8u, 8u) == 8u);

    using factor2 = factor_growth<2>;

    REQUIRE(factor2::growth_size(0u, 4u, memory_block::max_size()) == 4u);
    REQUIRE(factor2::growth_size(4u, 1u, memory_block::max_size()) == 8u);
    REQUIRE(factor2::growth_size(4u, 8u, memory_block::max_size()) == 12u);

    REQUIRE(factor2::shrink_size(4u, 2u) == 2u);
    REQUIRE(factor2::shrink_size(8u, 8u) == 8u);

    SECTION("whole_growth")
    {
        REQUIRE(detail::whole_growth<4u>::grow(2u) == 8u);
        REQUIRE(detail::whole_growth<3u>::grow(2u) == 6u);
    }
    SECTION("uneven_halfs_growth")
    {
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(1u) == 2u);   // 2.5
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(2u) == 5u);   // 5
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(3u) == 7u);   // 7.5
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(4u) == 10u);  // 10
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(5u) == 12u);  // 12.5
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(11u) == 27u); // 27.5
        REQUIRE(detail::uneven_halfs_growth<5u>::grow(32u) == 80u); // 80

        REQUIRE(detail::uneven_halfs_growth<11u>::grow(1u) == 5u);    // 5.5
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(2u) == 11u);   // 11
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(3u) == 16u);   // 16.5
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(4u) == 22u);   // 22
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(5u) == 27u);   // 27.5
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(11u) == 60u);  // 60.5
        REQUIRE(detail::uneven_halfs_growth<11u>::grow(32u) == 176u); // 176
    }
    SECTION("frac_growth")
    {
        REQUIRE(detail::frac_growth<5, 3>::grow(1u) == 2); // 1.6..
        REQUIRE(detail::frac_growth<5, 3>::grow(2u) == 3); // 3.3..
        REQUIRE(detail::frac_growth<5, 3>::grow(3u) == 5); // 5
        REQUIRE(detail::frac_growth<5, 3>::grow(4u) == 7); // 6.6..
        REQUIRE(detail::frac_growth<5, 3>::grow(5u) == 8); // 8.3..
    }
}
