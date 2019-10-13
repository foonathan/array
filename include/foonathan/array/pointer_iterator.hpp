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
    template <typename Tag, typename T>
    class pointer_iterator;

    namespace detail
    {
        struct pointer_iterator_ctor
        {};

        template <typename Tag, typename T>
        struct pointer_iterator_base
        {
            T* ptr_;

            explicit constexpr pointer_iterator_base(pointer_iterator_ctor, T* ptr) : ptr_(ptr) {}
            explicit constexpr pointer_iterator_base(Tag, T* ptr) : ptr_(ptr) {}
            template <class ContIter>
            explicit constexpr pointer_iterator_base(Tag, ContIter iter)
            : ptr_(iterator_to_pointer(iter))
            {}

            constexpr pointer_iterator_base() noexcept : ptr_(nullptr) {}
        };

        template <typename Tag, typename T>
        struct pointer_iterator_base<Tag, const T>
        {
            const T* ptr_;

            explicit constexpr pointer_iterator_base(pointer_iterator_ctor, const T* ptr)
            : ptr_(ptr)
            {}
            explicit constexpr pointer_iterator_base(Tag, const T* ptr) : ptr_(ptr) {}
            template <class ContIter>
            explicit constexpr pointer_iterator_base(Tag, ContIter iter)
            : ptr_(iterator_to_pointer(iter))
            {}

            constexpr pointer_iterator_base() noexcept : ptr_(nullptr) {}

            constexpr pointer_iterator_base(const pointer_iterator<Tag, T>& non_const) noexcept
            : ptr_(non_const.operator->())
            {}
        };
    } // namespace detail

    /// A `RandomAccessIterator` that is simply a type-safe wrapper over a pointer.
    template <typename Tag, typename T>
    class pointer_iterator : detail::pointer_iterator_base<Tag, T>
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = typename std::remove_cv<T>::type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        using detail::pointer_iterator_base<Tag, T>::pointer_iterator_base;

        //=== access ===//
        constexpr reference operator*() const noexcept
        {
            return *this->ptr_;
        }
        constexpr pointer operator->() const noexcept
        {
            return this->ptr_;
        }
        constexpr reference operator[](difference_type dist) const noexcept
        {
            return this->ptr_[dist];
        }

        //=== increment/decrement ===//
        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator++() noexcept
        {
            ++this->ptr_;
            return *this;
        }
        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator operator++(int) noexcept
        {
            auto save = *this;
            ++this->ptr_;
            return save;
        }

        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator--() noexcept
        {
            --this->ptr_;
            return *this;
        }
        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator operator--(int) noexcept
        {
            auto save = *this;
            --this->ptr_;
            return save;
        }

        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator+=(difference_type dist) noexcept
        {
            this->ptr_ += dist;
            return *this;
        }

        FOONATHAN_ARRAY_CONSTEXPR14 pointer_iterator& operator-=(difference_type dist) noexcept
        {
            this->ptr_ -= dist;
            return *this;
        }

        //=== addition/subtraction ===//
        friend constexpr pointer_iterator operator+(pointer_iterator iter,
                                                    difference_type  dist) noexcept
        {
            return pointer_iterator(detail::pointer_iterator_ctor{}, iter.ptr_ + dist);
        }
        friend constexpr pointer_iterator operator+(difference_type  dist,
                                                    pointer_iterator iter) noexcept
        {
            return iter + dist;
        }

        friend constexpr pointer_iterator operator-(pointer_iterator iter,
                                                    difference_type  dist) noexcept
        {
            return pointer_iterator(detail::pointer_iterator_ctor{}, iter.ptr_ - dist);
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
        template <typename U>
        friend struct is_contiguous_iterator;
    };

    template <typename Tag, typename T>
    struct is_contiguous_iterator<pointer_iterator<Tag, T>> : std::true_type
    {
        static constexpr T* to_pointer(pointer_iterator<Tag, T> iterator) noexcept
        {
            return iterator.operator->();
        }

        static constexpr pointer_iterator<Tag, T> to_iterator(T* pointer) noexcept
        {
            return pointer_iterator<Tag, T>(detail::pointer_iterator_ctor{}, pointer);
        }
    };
} // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_POINTER_ITERATOR_HPP_INCLUDED
