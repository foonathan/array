// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_EQUAL_CHECKER_HPP_INCLUDED
#define FOONATHAN_ARRAY_EQUAL_CHECKER_HPP_INCLUDED

#include <algorithm>

#include <catch.hpp>

namespace
{
    template <typename RaIt, typename RaIt2, typename Predicate, typename Printer>
    void check_equal(RaIt begin, RaIt end, RaIt2 begin_expected, RaIt2 end_expected, Predicate pred,
                     Printer print)
    {
        REQUIRE(end - begin == end_expected - begin_expected);

        if (!std::equal(begin, end, begin_expected, pred))
        {
            for (auto iter = begin; iter != end; ++iter)
                print(*iter);
            FAIL("not expected elements");
        }
    }
} // namespace

#endif // FOONATHAN_ARRAY_EQUAL_CHECKER_HPP_INCLUDED
