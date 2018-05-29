// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_HPP_INCLUDED

#include <utility>

#include <foonathan/array/block_storage.hpp>
#include <foonathan/array/growth_policy.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `BlockStorage` that uses the given `Heap` for (de-)allocation and the given `GrowthPolicy` to control the size.
        ///
        /// It does not have a small buffer optimization.
        template <class Heap, class GrowthPolicy>
        class block_storage_heap
        : block_storage_args_storage<block_storage_args_t<typename Heap::handle_type>>
        {
        public:
            using embedded_storage = std::false_type;
            using arg_type         = block_storage_args_t<typename Heap::handle_type>;

            //=== constructors/destructors ===//
            explicit block_storage_heap(const arg_type& arg) noexcept
            : block_storage_args_storage<arg_type>(arg)
            {
            }

            ~block_storage_heap() noexcept
            {
                deallocate_block(std::move(block_));
            }

            block_storage_heap(const block_storage_heap&) = delete;
            block_storage_heap& operator=(const block_storage_heap&) = delete;

            template <typename T>
            static void swap(block_storage_heap& lhs, block_view<T>& lhs_constructed,
                             block_storage_heap& rhs, block_view<T>& rhs_constructed) noexcept
            {
                std::swap(static_cast<block_storage_args_storage<arg_type>&>(lhs),
                          static_cast<block_storage_args_storage<arg_type>&>(rhs));
                std::swap(lhs.block_, rhs.block_);
                std::swap(lhs_constructed, rhs_constructed);
            }

            //=== reserve/shrink_to_fit ===//
            template <typename T>
            raw_pointer reserve(size_type min_additional_bytes, const block_view<T>& constructed)
            {
                auto new_size  = GrowthPolicy::growth_size(block_.size(), min_additional_bytes,
                                                          max_size(arguments()));
                auto new_block = allocate_block(new_size, alignof(T));
                return change_block(constructed, std::move(new_block));
            }

            template <typename T>
            raw_pointer shrink_to_fit(const block_view<T>& constructed)
            {
                auto byte_size = constructed.size() * sizeof(T);
                auto new_size  = GrowthPolicy::shrink_size(block_.size(), byte_size);
                auto new_block = allocate_block(new_size, alignof(T));
                return change_block(constructed, std::move(new_block));
            }

            //=== accessors ===//
            memory_block empty_block() const noexcept
            {
                return {};
            }

            const memory_block& block() const noexcept
            {
                return block_;
            }

            auto arguments() const noexcept -> decltype(this->stored_arguments())
            {
                return this->stored_arguments();
            }

            static size_type max_size(const arg_type& args) noexcept
            {
                auto&& handle = std::get<0>(args.args);
                return Heap::max_size(handle);
            }

        private:
            void deallocate_block(memory_block&& block) noexcept
            {
                auto&& handle = std::get<0>(this->stored_arguments().args);
                if (!block.empty())
                    Heap::deallocate(handle, std::move(block));
            }

            memory_block allocate_block(size_type size, size_type alignment)
            {
                if (size == 0)
                    return memory_block();
                else
                {
                    auto&& handle = std::get<0>(this->stored_arguments().args);
                    return Heap::allocate(handle, size, alignment);
                }
            }

            template <typename T>
            raw_pointer change_block(const block_view<T>& constructed, memory_block&& new_block)
            {
                raw_pointer end;
                try
                {
                    end = uninitialized_destructive_move(constructed.begin(), constructed.end(),
                                                         new_block);
                }
                catch (...)
                {
                    deallocate_block(std::move(new_block));
                    throw;
                }

                deallocate_block(std::move(block_));
                block_ = new_block;

                return end;
            }

            memory_block block_;
        };
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_HEAP_HPP_INCLUDED
