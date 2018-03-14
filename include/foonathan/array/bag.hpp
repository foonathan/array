// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BAG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BAG_HPP_INCLUDED

#include <foonathan/array/memory_block.hpp>
#include <foonathan/array/pointer_iterator.hpp>
#include <foonathan/array/raw_storage.hpp>

namespace foonathan
{
    namespace array
    {
        /// A bag of elements.
        ///
        /// It is the most basic collection of elements.
        /// Insert and erase are (amortized) O(1), but the order of elements is not determined.
        template <typename T, class BlockStorage>
        class bag
        {
            struct iter_tag
            {
            };

        public:
            using value_type    = T;
            using block_storage = BlockStorage;

            using iterator       = pointer_iterator<iter_tag, T>;
            using const_iterator = pointer_iterator<iter_tag, const T>;

            bag() : bag(typename block_storage::arg_type{}) {}

            bag(typename block_storage::arg_type args)
            : storage_(std::move(args)), end_(storage_.block().begin())
            {
            }

            bag(const bag& other)
            : storage_(other.storage_.arguments()), end_(storage_.block().begin())
            {
                insert_range(other.begin(), other.end());
            }

            bag(bag&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
            : storage_(other.storage_.arguments()), end_(storage_.block().begin())
            {
                storage_.move_construct(std::move(other.storage_), other.data(), other.data_end());
            }

            ~bag() noexcept
            {
                destroy_range(data(), data_end());
            }

            bag& operator=(const bag& other)
            {
                bag tmp(other);
                swap(*this, tmp);
                return *this;
            }

            bag& operator=(bag&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
            {
                bag tmp(std::move(other));
                swap(*this, tmp);
                return *this;
            }

            friend void swap(bag& lhs,
                             bag& rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
            {
                lhs.storage_.swap(lhs.data(), lhs.data_end(), rhs.storage_, rhs.data(),
                                  rhs.data_end());
            }

            //=== element access ===//
            T* data() noexcept
            {
                return to_pointer<T>(storage_.block().begin());
            }
            const T* data() const noexcept
            {
                return to_pointer<const T>(storage_.block().begin());
            }

            T* data_end() noexcept
            {
                return to_pointer<T>(end_);
            }
            const T* data_end() const noexcept
            {
                return to_pointer<const T>(end_);
            }

            //=== iterators ===//
            iterator begin() noexcept
            {
                return iterator(iter_tag(), data());
            }
            const_iterator begin() const noexcept
            {
                return const_iterator(iter_tag(), data());
            }
            const_iterator cbegin() const noexcept
            {
                return const_iterator(iter_tag(), data());
            }

            iterator end() noexcept
            {
                return iterator(iter_tag(), data_end());
            }
            const_iterator end() const noexcept
            {
                return const_iterator(iter_tag(), data_end());
            }
            const_iterator cend() const noexcept
            {
                return const_iterator(iter_tag(), data_end());
            }

            //=== capacity ===//
            bool empty() const noexcept
            {
                return !storage_.block();
            }

            size_type size() const noexcept
            {
                return end_ - storage_.block().memory;
            }

            static size_type max_size() noexcept
            {
                return block_storage::max_capacity;
            }

            size_type capacity() const noexcept
            {
                return storage_.block().size;
            }

            void reserve(size_type new_capacity)
            {
                if (new_capacity > capacity())
                    reserve_impl(new_capacity - capacity());
            }

            void shrink_to_fit()
            {
                storage_.shrink_to_fit(data(), data_end());
            }

            //=== modifiers ===//
            void clear() noexcept
            {
                destroy_range(data(), data_end());
                end_ = storage_.block().memory;
            }

            template <typename... Args>
            void emplace(Args&&... args)
            {
                if (need_reserve(1u))
                    reserve_impl(size_type(1u));
                construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
            }

            void insert(const T& value)
            {
                emplace(value);
            }
            void insert(T&& value)
            {
                emplace(std::move(value));
            }

            template <typename InputIt>
            void insert_range(InputIt begin, InputIt end)
            {
                insert_range_impl(typename std::iterator_traits<InputIt>::iterator_category{},
                                  begin, end);
            }

            iterator erase(const_iterator pos) noexcept
            {
                // const_cast is fine, pointer was never const to begin with
                auto mutable_pos = iterator(iter_tag{}, const_cast<T*>(pos.operator->()));

                using std::swap;
                swap(*mutable_pos, *std::prev(end())); // move element to the end
                // note: this does a self-swap if we're removing the last element

                // destroy the last element
                end_ -= sizeof(T);
                destroy_object(data_end());

                // return an iterator to the same position,
                // this is the next element that should be visited
                return mutable_pos;
            }

        private:
            template <typename InputIt>
            void insert_range_impl(std::input_iterator_tag, InputIt begin, InputIt end)
            {
                for (auto cur = begin; cur != end; ++cur)
                    insert(*cur);
            }

            template <typename ForwardIt>
            void insert_range_impl(std::forward_iterator_tag, ForwardIt begin, ForwardIt end)
            {
                auto needed = size_type(std::distance(begin, end));
                if (need_reserve(needed))
                    reserve_impl(needed);

                for (auto cur = begin; cur != end; ++cur)
                {
                    construct_object(end_, *cur);
                    end_ += sizeof(T);
                }
            }

            bool need_reserve(size_type needed) const noexcept
            {
                auto space = size_type(storage_.block().end() - end_);
                return space < needed;
            }

            void reserve_impl(size_type additional)
            {
                end_ = storage_.reserve(additional, data(), data_end());
            }

            BlockStorage storage_;
            raw_pointer  end_;
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_BAG_HPP_INCLUDED
