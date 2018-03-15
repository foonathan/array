// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_VIEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_VIEW_HPP_INCLUDED

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
            template <typename ContIter>
            constexpr block_view(
                ContIter begin, ContIter end,
                typename std::enable_if<
                    is_contiguous_iterator<ContIter>::value
                        && std::is_same<T*, decltype(is_contiguous_iterator<ContIter>::to_pointer(
                                                begin))>::value,
                    int>::type = 0)
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
                return memory_block(from_pointer(begin_), from_pointer(end_));
            }

        private:
            T* begin_;
            T* end_;
        };

        /// \returns A view to the range `[data, data + size)`.
        template <typename T>
        block_view<T> make_block_view(T* data, size_type size)
        {
            return block_view<T>(data, size);
        }

        /// \returns A view to the range `[begin, end)`.
        /// \notes This function does not participate in overload resolution, unless they are contiguous iterators.
        template <typename ContIter>
        auto make_block_view(ContIter begin, ContIter end) -> block_view<
            typename std::remove_reference<decltype(*iterator_to_pointer(begin))>::type>
        {
            return {begin, end};
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        block_view<T> make_block_view(T (&array)[N])
        {
            return block_view<T>(array);
        }

        /// A lightweight view into an array.
        ///
        /// This is an [array::block_view]() where the objects have a given ordering
        /// and it makes sense to ask for the `n`-th element.
        /// \notes Inheritance is only used to easily inherit all functionality as well as allow conversion without a user-defined conversion.
        /// Slicing is permitted and works, but the type isn't meant to be used polymorphically.
        template <typename T>
        class array_view : public block_view<T>
        {
        public:
            using block_view<T>::block_view;

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
        };

        /// \returns A view to the range `[data, data + size)`.
        template <typename T>
        array_view<T> make_array_view(T* data, size_type size)
        {
            return array_view<T>(data, size);
        }

        /// \returns A view to the range `[begin, end)`.
        /// \notes This function does not participate in overload resolution, unless they are contiguous iterators.
        template <typename ContIter>
        auto make_array_view(ContIter begin, ContIter end)
            -> array_view<decltype(*iterator_to_pointer(begin))>
        {
            return array_view<decltype(*iterator_to_pointer(begin))>(begin, end);
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        array_view<T> make_array_view(T (&array)[N])
        {
            return array_view<T>(array);
        }
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_VIEW_HPP_INCLUDED
