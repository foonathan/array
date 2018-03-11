// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/growth_policy.hpp>

#include <catch.hpp>

using namespace foonathan::array;

TEST_CASE("no_extra_growth", "[GrowthPolicy]")
{
    REQUIRE(no_extra_growth::growth_size(0u, 4u) == 4u);
    REQUIRE(no_extra_growth::growth_size(4u, 8u) == 12u);

    REQUIRE(no_extra_growth::shrink_size(4u, 2u) == 2u);
    REQUIRE(no_extra_growth::shrink_size(8u, 8u) == 8u);
}

TEST_CASE("factor_growth", "[GrowthPolicy]")
{
    using factor1dot5 = factor_growth<3, 2>;
    static_assert(factor1dot5::growth_factor == 1.5f, "");

    REQUIRE(factor1dot5::growth_size(0u, 4u) == 4u);
    REQUIRE(factor1dot5::growth_size(4u, 1u) == 6u);
    REQUIRE(factor1dot5::growth_size(5u, 1u) == 7u);
    REQUIRE(factor1dot5::growth_size(4u, 8u) == 12u);

    REQUIRE(factor1dot5::shrink_size(4u, 2u) == 2u);
    REQUIRE(factor1dot5::shrink_size(8u, 8u) == 8u);

    using factor2 = factor_growth<2>;
    static_assert(factor2::growth_factor == 2, "");

    REQUIRE(factor2::growth_size(0u, 4u) == 4u);
    REQUIRE(factor2::growth_size(4u, 1u) == 8u);
    REQUIRE(factor2::growth_size(4u, 8u) == 12u);

    REQUIRE(factor2::shrink_size(4u, 2u) == 2u);
    REQUIRE(factor2::shrink_size(8u, 8u) == 8u);
}
