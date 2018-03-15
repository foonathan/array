// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED
#define FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED

#include <iterator>

#include <foonathan/array/config.hpp>
#include <foonathan/array/contiguous_iterator.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `RandomAccessIterator` that is simply a type-safe wrapper over a pointer.
        template <typename Tag, typename T>
        class pointer_iterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = T*;
            using reference         = T&;

            /// \effects Creates an iterator from the specified pointer.
            /// \notes You can make the `Tag` constructor `private` to prevent anybody from using that constructor,
            /// except `friend`s of the `Tag`.
            explicit constexpr pointer_iterator(Tag, pointer ptr) : ptr_(ptr) {}

            constexpr pointer_iterator() noexcept : ptr_(nullptr) {}

            constexpr pointer_iterator(
                const pointer_iterator<Tag, typename std::remove_const<T>::type>&
                    non_const) noexcept
            : ptr_(non_const.operator->())
            {
            }

            //=== access ===//
            constexpr reference operator*() const noexcept
            {
                return *ptr_;
            }
            constexpr pointer operator->() const noexcept
            {
                return ptr_;
            }
            constexpr reference operator[](difference_type dist) const noexcept
            {
                return ptr_[dist];
            }

            //=== increment/decrement ===//
            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator++() noexcept
            {
                ++ptr_;
                return *this;
            }
            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator operator++(int)noexcept
            {
                auto save = *this;
                ++ptr_;
                return save;
            }

            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator--() noexcept
            {
                --ptr_;
                return *this;
            }
            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator operator--(int)noexcept
            {
                auto save = *this;
                --ptr_;
                return save;
            }

            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator+=(difference_type dist) noexcept
            {
                ptr_ += dist;
                return *this;
            }

            FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator-=(difference_type dist) noexcept
            {
                ptr_ -= dist;
                return *this;
            }

            //=== addition/subtraction ===//
            friend constexpr pointer_iterator operator+(pointer_iterator iter,
                                                        difference_type  dist) noexcept
            {
                return pointer_iterator(iter.ptr_ + dist);
            }
            friend constexpr pointer_iterator operator+(difference_type  dist,
                                                        pointer_iterator iter) noexcept
            {
                return iter + dist;
            }

            friend constexpr pointer_iterator operator-(pointer_iterator iter,
                                                        difference_type  dist) noexcept
            {
                return pointer_iterator(iter.ptr_ - dist);
            }
            friend constexpr pointer_iterator operator-(difference_type  dist,
                                                        pointer_iterator iter) noexcept
            {
                return iter - dist;
            }

            friend constexpr difference_type operator-(pointer_iterator lhs,
                                                       pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ - rhs.ptr_;
            }

            //=== comparision ===//
            friend constexpr bool operator==(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ == rhs.ptr_;
            }
            friend constexpr bool operator!=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ != rhs.ptr_;
            }

            friend constexpr bool operator<(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ < rhs.ptr_;
            }
            friend constexpr bool operator>(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ > rhs.ptr_;
            }
            friend constexpr bool operator<=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ <= rhs.ptr_;
            }
            friend constexpr bool operator>=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ >= rhs.ptr_;
            }

        private:
            pointer_iterator(pointer ptr) : ptr_(ptr) {}

            T* ptr_;
        };

        template <typename Tag, typename T>
        struct is_contiguous_iterator<pointer_iterator<Tag, T>> : std::true_type
        {
            static constexpr T* to_pointer(pointer_iterator<Tag, T> iterator) noexcept
            {
                return iterator.operator->();
            }
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED
