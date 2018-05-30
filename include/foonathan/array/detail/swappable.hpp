// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_SWAPPABLE_HPP_INCLUDED
#define FOONATHAN_ARRAY_SWAPPABLE_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <foonathan/array/config.hpp>

namespace foonathan
{
    namespace array
    {
        namespace swap_detail
        {
            using std::swap;

            template <typename T>
            auto adl_swap(T& lhs, T& rhs) noexcept(noexcept(swap(lhs, rhs)))
                -> decltype(void(swap(lhs, rhs)))
            {
                return swap(lhs, rhs);
            }

            // MSVC issue with std::declval
            template <typename T>
            constexpr T& declval() noexcept;

            template <typename T, typename = void>
            struct is_swappable : std::false_type
            {
            };
            template <typename T>
            struct is_swappable<T, decltype(adl_swap(declval<T>(), declval<T>()))> : std::true_type
            {
            };

            template <typename T, bool Swappable>
            struct is_nothrow_swappable : std::false_type
            {
            };

#if defined(_MSC_VER) && _MSC_VER <= 1900
            // can't get it to work here
            template <typename T>
            struct is_nothrow_swappable<T, true> : std::false_type
            {
            };
#else
            template <typename T>
            struct is_nothrow_swappable<T, true>
            : std::integral_constant<bool, noexcept(adl_swap(declval<T>(), declval<T>()))>
            {
            };
#endif
        } // namespace swap_detail

        namespace detail
        {
            template <typename T>
            auto adl_swap(T& lhs, T& rhs) noexcept(noexcept(swap_detail::adl_swap(lhs, rhs)))
                -> decltype(swap_detail::adl_swap(lhs, rhs))
            {
                return swap_detail::adl_swap(lhs, rhs);
            }

            template <typename T>
            struct is_swappable : swap_detail::is_swappable<T>
            {
#if FOONATHAN_ARRAY_HAS_SWAPPABLE
                static_assert(is_swappable<T>::value == std::is_swappable<T>::value,
                              "badly implemented trait");
#endif
            };

            template <typename T>
            struct is_nothrow_swappable
            : swap_detail::is_nothrow_swappable<T, swap_detail::is_swappable<T>::value>
            {
#if FOONATHAN_ARRAY_HAS_SWAPPABLE
                static_assert(is_nothrow_swappable<T>::value == std::is_nothrow_swappable<T>::value,
                              "badly implemented trait");
#endif
            };
        } // namespace detail
    }     // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_SWAPPABLE_HPP_INCLUDED
