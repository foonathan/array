// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/contiguous_iterator.hpp>

#include <catch.hpp>

using namespace foonathan::array;

namespace
{
    template <typename T>
    std::true_type valid_iter_to_pointer(int, const T& iter,
                                         decltype(iterator_to_pointer(iter)) = nullptr);

    template <typename T>
    std::false_type valid_iter_to_pointer(short, const T&);
}

TEST_CASE("is_contiguous_iterator", "[core]")
{
    REQUIRE(!is_contiguous_iterator<int>::value);
    REQUIRE(is_contiguous_iterator<int*>::value);
    REQUIRE(is_contiguous_iterator<const int*>::value);
    REQUIRE(is_contiguous_iterator<volatile int*>::value);

    auto obj  = 0;
    auto iter = &obj;
    REQUIRE(iterator_to_pointer(iter) == &obj);

    // sfinae test
    REQUIRE(decltype(valid_iter_to_pointer(0, iter))::value);
    REQUIRE(!decltype(valid_iter_to_pointer(0, obj))::value);
}
