// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_VIEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_VIEW_HPP_INCLUDED

#include <initializer_list>

#include <foonathan/array/memory_block.hpp>
#include <foonathan/array/pointer_iterator.hpp>

namespace foonathan
{
    namespace array
    {
        /// A lightweight view into a memory block containing objects of the given type.
        ///
        /// The elements can be mutated if `T` is not `const`.
        /// Use `block_view<const T>` to have a view to `const`.
        ///
        /// The objects don't have any particular ordering, so it doesn't provide an index operator.
        /// Use [array::array_view]() where that is required.
        /// \notes Write an implicit conversion operator for containers that have contiguous storage,
        /// and specialize the [array::block_traits]() if it doesn't provide a `value_type` typedef.
        template <typename T>
        class block_view
        {
            class iterator_tag
            {
                constexpr iterator_tag() {}

                friend block_view;
            };

        public:
            using value_type = typename std::remove_cv<T>::type;
            using iterator   = pointer_iterator<iterator_tag, T>;

            //=== constructors ===//
            /// \effects Creates an empty view.
            constexpr block_view() noexcept : begin_(nullptr), end_(nullptr) {}

            /// \effects Creates a view on the [array::memory_block]().
            /// \requires The [array::memory_block]() must contain objects of type `T`.
            explicit constexpr block_view(const memory_block& block) noexcept
            : begin_(to_pointer<T>(block.begin())), end_(to_pointer<T>(block.end()))
            {
            }

            /// \effects Creates a view on the range `[data, data + size)`.
            constexpr block_view(T* data, size_type size) : begin_(data), end_(data + size) {}

            /// \effects Creates a view on the range `[begin, end)`.
            /// \notes This constructor only participates in overload resolution if `ContIter` is a contiguous iterator,
            /// and the value type is the same.
            template <typename ContIter,
                      typename = typename std::enable_if<
                          std::is_same<T, contiguous_iterator_value_type<ContIter>>::value>::type>
            constexpr block_view(ContIter begin, ContIter end)
            : begin_(iterator_to_pointer(begin)), end_(iterator_to_pointer(end))
            {
            }

            /// \effects Creates a view on the given array.
            template <std::size_t N>
            constexpr block_view(T (&array)[N]) noexcept : begin_(array), end_(array + N)
            {
            }

            /// \effects Creates a view on the elements of the initializer list.
            /// \notes This constructor only participates in overload resolution, if `const U` is the same as `T`,
            /// i.e. `T` is `const` and `U` is the non-`const` version of it.
            template <typename U,
                      typename = typename std::enable_if<std::is_same<const U, T>::value>::type>
            constexpr block_view(std::initializer_list<U> list) noexcept
            : begin_(list.begin()), end_(list.end())
            {
            }

            /// \effects Converts a view to non-const to a view to const.
            /// \notes This constructor only participates in overload resolution, if `const U` is the same as `T`,
            /// i.e. `T` is `const` and `U` is the non-`const` version of it.
            template <typename U,
                      typename = typename std::enable_if<std::is_same<const U, T>::value>::type>
            constexpr block_view(const block_view<U>& other) noexcept
            : begin_(other.data()), end_(other.data_end())
            {
            }

            //=== accessors ===//
            /// \returns Whether or not the block is empty.
            constexpr bool empty() const noexcept
            {
                return begin_ == end_;
            }

            /// \returns The number of elements in the view.
            constexpr size_type size() const noexcept
            {
                return size_type(end_ - begin_);
            }

            /// \returns A pointer to the first element.
            constexpr T* data() const noexcept
            {
                return begin_;
            }

            /// \returns A pointer one past the last element.
            constexpr T* data_end() const noexcept
            {
                return end_;
            }

            /// \returns An iterator to the first element.
            constexpr iterator begin() const noexcept
            {
                return iterator(iterator_tag(), data());
            }

            /// \returns An iterator one past the last element.
            constexpr iterator end() const noexcept
            {
                return iterator(iterator_tag(), data_end());
            }

            /// \returns The memory block it views.
            /// \notes This function only participates in overload resolution if `T` is not `const`.
            template <typename Dummy = void,
                      typename = typename std::enable_if<!std::is_const<T>::value, Dummy>::type>
            constexpr memory_block block() const noexcept
            {
                return memory_block(to_raw_pointer(begin_), to_raw_pointer(end_));
            }

        private:
            T* begin_;
            T* end_;
        };

        /// \returns A view to the range `[data, data + size)`.
        template <typename T>
        constexpr block_view<T> make_block_view(T* data, size_type size) noexcept
        {
            return block_view<T>(data, size);
        }

        /// \returns A view to the range `[begin, end)`.
        /// \notes This function does not participate in overload resolution, unless they are contiguous iterators.
        template <typename ContIter>
        constexpr auto make_block_view(ContIter begin, ContIter end) noexcept
            -> block_view<contiguous_iterator_value_type<ContIter>>
        {
            return {begin, end};
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        constexpr block_view<T> make_block_view(T (&array)[N]) noexcept
        {
            return block_view<T>(array);
        }

        namespace block_traits_detail
        {
            template <typename T, typename = void>
            struct default_block_traits
            {
            };

            template <typename T>
            using cv_value_type =
                typename std::conditional<std::is_const<T>::value, const typename T::value_type,
                                          typename T::value_type>::type;

            template <typename T>
            struct default_block_traits<
                T, typename std::enable_if<std::is_convertible<
                       T, foonathan::array::block_view<cv_value_type<T>>>::value>::type>
            {
                using value_type = cv_value_type<T>;
                using block_view = foonathan::array::block_view<cv_value_type<T>>;
            };
        } // namespace block_traits_detail

        /// Traits for types that are blocks.
        ///
        /// A type is a block if it has a valid specialization and is convertible to an [array::block_view]().
        /// The traits must be specialized for `T` and `const T` to allow propagating the const-ness.
        ///
        /// The default specialization works for all types having a `value_type` typedef,
        /// and where `T` is convertible to `block_view<T::value_type>`,
        /// and `const T` convertible to `block_view<const T::value_type>`.
        template <typename T>
        struct block_traits : block_traits_detail::default_block_traits<T>
        {
        };

        /// Specialization for arrays.
        template <typename T, std::size_t N>
        struct block_traits<T[N]>
        {
            using value_type = T;
            using block_view = foonathan::array::block_view<T>;
        };

        /// Specialization for [std::initializer_list]().
        template <typename T>
        struct block_traits<std::initializer_list<T>>
        {
            using value_type = const T;
            using block_view = foonathan::array::block_view<const T>;
        };

        /// Specialization for [std::initializer_list]().
        template <typename T>
        struct block_traits<const std::initializer_list<T>>
        {
            using value_type = const T;
            using block_view = foonathan::array::block_view<const T>;
        };

        /// Calculates the value type of the [array::block_view]() the type is convertible to.
        /// Pass in a `const` type to get the value type for a `const` object.
        /// \notes This typedef is ill-formed if the type is not actually a block.
        template <class Block>
        using block_value_type =
            typename block_traits<typename std::remove_reference<Block>::type>::value_type;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_VIEW_HPP_INCLUDED
