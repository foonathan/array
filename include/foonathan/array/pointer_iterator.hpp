// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED
#define FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED

#include <iterator>

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
            explicit pointer_iterator(Tag, pointer ptr) : ptr_(ptr) {}

            pointer_iterator() noexcept : ptr_(nullptr) {}

            pointer_iterator(
                pointer_iterator<Tag, typename std::remove_const<T>::type> non_const) noexcept
            : ptr_(non_const.operator->())
            {
            }

            //=== access ===//
            reference operator*() const noexcept
            {
                return *ptr_;
            }
            pointer operator->() const noexcept
            {
                return ptr_;
            }
            reference operator[](difference_type dist) const noexcept
            {
                return ptr_[dist];
            }

            //=== increment/decrement ===//
            pointer_iterator& operator++() noexcept
            {
                ++ptr_;
                return *this;
            }
            pointer_iterator operator++(int)noexcept
            {
                auto save = *this;
                ++ptr_;
                return save;
            }

            pointer_iterator& operator--() noexcept
            {
                --ptr_;
                return *this;
            }
            pointer_iterator operator--(int)noexcept
            {
                auto save = *this;
                --ptr_;
                return save;
            }

            pointer_iterator& operator+=(difference_type dist) noexcept
            {
                ptr_ += dist;
                return *this;
            }

            pointer_iterator& operator-=(difference_type dist) noexcept
            {
                ptr_ -= dist;
                return *this;
            }

            //=== addition/subtraction ===//
            friend pointer_iterator operator+(pointer_iterator iter, difference_type dist) noexcept
            {
                iter += dist;
                return iter;
            }
            friend pointer_iterator operator+(difference_type dist, pointer_iterator iter) noexcept
            {
                return iter + dist;
            }

            friend pointer_iterator operator-(pointer_iterator iter, difference_type dist) noexcept
            {
                iter -= dist;
                return iter;
            }
            friend pointer_iterator operator-(difference_type dist, pointer_iterator iter) noexcept
            {
                return iter - dist;
            }

            friend difference_type operator-(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ - rhs.ptr_;
            }

            //=== comparision ===//
            friend bool operator==(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ == rhs.ptr_;
            }
            friend bool operator!=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ != rhs.ptr_;
            }

            friend bool operator<(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ < rhs.ptr_;
            }
            friend bool operator>(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ > rhs.ptr_;
            }
            friend bool operator<=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ <= rhs.ptr_;
            }
            friend bool operator>=(pointer_iterator lhs, pointer_iterator rhs) noexcept
            {
                return lhs.ptr_ >= rhs.ptr_;
            }

        private:
            T* ptr_;
        };

        template <typename Tag, typename T>
        struct is_contiguous_iterator<pointer_iterator<Tag, T>> : std::true_type
        {
            static T* to_pointer(pointer_iterator<Tag, T> iterator) noexcept
            {
                return iterator.operator->();
            }
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED
