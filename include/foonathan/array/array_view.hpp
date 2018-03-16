// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_ARRAY_VIEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_ARRAY_VIEW_HPP_INCLUDED

#include <foonathan/array/block_view.hpp>

namespace foonathan
{
    namespace array
    {
        /// A lightweight view into an array.
        ///
        /// This is an [array::block_view]() where the objects have a given ordering
        /// and it makes sense to ask for the `n`-th element.
        /// \notes If you don't need the special functions like array access, use [array::block_view]() instead.
        /// \notes Inheritance is only used to easily inherit all functionality as well as allow conversion without a user-defined conversion.
        /// Slicing is permitted and works, but the type isn't meant to be used polymorphically.
        /// \notes Write an implicit conversion operator for containers that have contiguous storage with ordering,
        /// and specialize the [array::block_traits]() if it does not provide a `value_type` typedef.
        template <typename T>
        class array_view : public block_view<T>
        {
        public:
            using block_view<T>::block_view;

            /// \effects Creates a view to the block.
            /// \notes This gives the block ordering which may not be there.
            explicit constexpr array_view(const block_view<T>& block) noexcept
            : block_view<T>(block)
            {
            }

            //=== array access ===//
            /// \returns The `i`-th element of the array.
            /// \requires `i < size()`.
            constexpr T& operator[](size_type i) const noexcept
            {
                return this->data()[i];
            }

            /// \returns The first element of the array.
            /// \requires `!empty()`.
            constexpr T& front() const noexcept
            {
                return this->data()[0];
            }

            /// \returns The last element of the array.
            /// \requires `!empty()`.
            constexpr T& back() const noexcept
            {
                return this->data()[this->size() - 1u];
            }

            /// \returns A slice of the array starting at `pos` containing `n` elements.
            /// \requires `[data() + pos, data() + pos + n)` must be a valid range.
            constexpr array_view<T> slice(size_type pos, size_type n) const noexcept
            {
                return array_view<T>(this->data() + pos, n);
            }

            /// \returns A slice of the array starting at `begin` and containing `n` elements.
            /// \requires `[begin, begin + n)` must be a valid range.
            constexpr array_view<T> slice(typename block_view<T>::iterator begin, size_type n) const
                noexcept
            {
                return array_view<T>(begin, begin + std::ptrdiff_t(n));
            }
        };

        /// \returns The array view viewing the given block.
        /// \notes This gives the block ordering which may not be there.
        template <typename T>
        constexpr array_view<T> make_array_view(const block_view<T>& block) noexcept
        {
            return array_view<T>(block);
        }

        /// \returns A view to the range `[data, data + size)`.
        template <typename T>
        constexpr array_view<T> make_array_view(T* data, size_type size) noexcept
        {
            return array_view<T>(data, size);
        }

        /// \returns A view to the range `[begin, end)`.
        /// \notes This function does not participate in overload resolution, unless they are contiguous iterators.
        template <typename ContIter>
        constexpr auto make_array_view(ContIter begin, ContIter end) noexcept
            -> array_view<decltype(*iterator_to_pointer(begin))>
        {
            return array_view<decltype(*iterator_to_pointer(begin))>(begin, end);
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        constexpr array_view<T> make_array_view(T (&array)[N]) noexcept
        {
            return array_view<T>(array);
        }
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_ARRAY_VIEW_HPP_INCLUDED
