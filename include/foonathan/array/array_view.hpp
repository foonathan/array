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
        /// This is an [array::block_view]() where the objects have "some" order
        /// and it makes sense to ask for the `n`-th element.
        /// \notes If you don't need the special functions like array access, use [array::block_view]() instead.
        /// \notes If you require a view into a *sorted* array, use [array::sorted_view]() instead.
        /// \notes Inheritance is only used to easily inherit all functionality as well as allow conversion without a user-defined conversion.
        /// Slicing is permitted and works, but the type isn't meant to be used polymorphically.
        /// \notes Write an implicit conversion operator for containers that have contiguous storage with ordering,
        /// and specialize the [array::block_traits]() if it does not provide a `value_type` typedef.
        template <typename T>
        class array_view : public block_view<T>
        {
        public:
            /// \effects Creates an empty view.
            constexpr array_view() noexcept = default;

            /// \effects Creates a view on the [array::memory_block]().
            /// \requires The [array::memory_block]() must contain objects of type `T`.
            explicit constexpr array_view(const memory_block& block) noexcept : block_view<T>(block)
            {
            }

            /// \effects Creates a view on the range `[data, data + size)`.
            constexpr array_view(T* data, size_type size) : block_view<T>(data, size) {}

            /// \effects Creates a view on the range `[begin, end)`.
            /// \notes This constructor only participates in overload resolution if `ContIter` is a contiguous iterator,
            /// and the value type is the same.
            template <typename ContIter,
                      typename = typename std::enable_if<
                          std::is_same<T, contiguous_iterator_value_type<ContIter>>::value>::type>
            constexpr array_view(ContIter begin, ContIter end) : block_view<T>(begin, end)
            {
            }

            /// \effects Creates a view on the given array.
            template <std::size_t N>
            constexpr array_view(T (&array)[N]) noexcept : block_view<T>(array)
            {
            }

            /// \effects Creates a view on the elements of the initializer list.
            /// \notes This constructor only participates in overload resolution, if `const U` is the same as `T`,
            /// i.e. `T` is `const` and `U` is the non-`const` version of it.
            template <typename U,
                      typename = typename std::enable_if<std::is_same<const U, T>::value>::type>
            constexpr array_view(std::initializer_list<U> list) noexcept : block_view<T>(list)
            {
            }

            /// \effects Converts a view to non-const to a view to const.
            /// \notes This constructor only participates in overload resolution, if `const U` is the same as `T`,
            /// i.e. `T` is `const` and `U` is the non-`const` version of it.
            template <typename U,
                      typename = typename std::enable_if<std::is_same<const U, T>::value>::type>
            constexpr array_view(const array_view<U>& other) noexcept : block_view<T>(other)
            {
            }

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
            -> array_view<contiguous_iterator_value_type<ContIter>>
        {
            return {begin, end};
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        constexpr array_view<T> make_array_view(T (&array)[N]) noexcept
        {
            return array_view<T>(array);
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_ARRAY_VIEW_HPP_INCLUDED
