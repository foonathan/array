// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BYTE_VIEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BYTE_VIEW_HPP_INCLUDED

#include <foonathan/array/array_view.hpp>

namespace foonathan
{
    namespace array
    {
        namespace detail
        {
            template <typename T>
            struct select_byte_view
            {
                using type = byte;
            };

            template <typename T>
            struct select_byte_view<const T>
            {
                using type = const byte;
            };

            template <typename T>
            struct select_byte_view<volatile T>
            {
                using type = volatile byte;
            };

            template <typename T>
            struct select_byte_view<const volatile T>
            {
                using type = const volatile byte;
            };

            template <typename Byte>
            using enable_byte_view = typename std::enable_if<
                std::is_same<byte, typename std::remove_cv<Byte>::type>::value>::type;
        } // namespace detail

        /// A lightweight byte-wise view into a memory block.
        ///
        /// This is an `array_view<cv byte>`, where cv-qualifiers are taken from `T`.
        template <typename T>
        using make_byte_view_t = array_view<typename detail::select_byte_view<T>::type>;

        /// \returns A byte-wise view into the given block.
        template <typename T>
        constexpr make_byte_view_t<T> byte_view(const block_view<T>& view) noexcept
        {
            return make_byte_view_t<T>(to_raw_pointer(view.data()),
                                       to_raw_pointer(view.data_end()));
        }

        /// \returns A reinterpretation of the byte view as the given type.
        template <typename T, typename Byte, typename = detail::enable_byte_view<Byte>>
        constexpr block_view<T> reinterpret_block(const array_view<Byte>& view) noexcept
        {
            return block_view<T>(to_pointer<T>(view.data()), to_pointer<T>(view.data_end()));
        }

        /// \returns A reinterpretation of the byte view as the given type.
        template <typename T, typename Byte, typename = detail::enable_byte_view<Byte>>
        constexpr array_view<T> reinterpret_array(const array_view<Byte>& view) noexcept
        {
            return make_array_view(reinterpret_block<T>(view));
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BYTE_VIEW_HPP_INCLUDED
