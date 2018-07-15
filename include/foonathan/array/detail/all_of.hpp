// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_DETAIL_ALL_OF_HPP_INCLUDED
#define FOONATHAN_ARRAY_DETAIL_ALL_OF_HPP_INCLUDED

#include <type_traits>

namespace foonathan
{
    namespace array
    {
        namespace detail
        {
            template <bool... Bs>
            struct bool_sequence
            {
            };

            template <bool... Bs>
            using all_of = std::is_same<bool_sequence<Bs...>, bool_sequence<(true || Bs)...>>;

            template <bool... Bs>
            using none_of = std::is_same<bool_sequence<Bs...>, bool_sequence<(false && Bs)...>>;

            template <bool... Bs>
            using any_of = std::integral_constant<bool, !none_of<Bs...>::value>;
        } // namespace detail
    }     // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_DETAIL_ALL_OF_HPP_INCLUDED
