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
                return memory_block(as_raw_pointer(begin_), as_raw_pointer(end_));
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
        constexpr auto make_block_view(ContIter begin, ContIter end) noexcept -> block_view<
            typename std::remove_reference<decltype(*iterator_to_pointer(begin))>::type>
        {
            return {begin, end};
        }

        /// \returns A view to the array.
        template <typename T, std::size_t N>
        constexpr block_view<T> make_block_view(T (&array)[N]) noexcept
        {
            return block_view<T>(array);
        }

        /// A lightweight view into an array.
        ///
        /// This is an [array::block_view]() where the objects have a given ordering
        /// and it makes sense to ask for the `n`-th element.
        /// \notes If you don't need the special functions like array access, use [array::block_view]() instead.
        /// \notes Inheritance is only used to easily inherit all functionality as well as allow conversion without a user-defined conversion.
        /// Slicing is permitted and works, but the type isn't meant to be used polymorphically.
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
                return array_view<T>(begin, begin + n);
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
            return make_byte_view_t<T>(as_raw_pointer(view.data()),
                                       as_raw_pointer(view.data_end()));
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
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_VIEW_HPP_INCLUDED
