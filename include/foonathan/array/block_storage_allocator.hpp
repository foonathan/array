// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED

#include <memory>

#include <foonathan/array/block_storage.hpp>
#include <foonathan/array/growth_policy.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `BlockStorage` that uses an `Allocator` for allocation.
        ///
        /// It will rebind the `Allocator` to `std::byte` and basically ignore all functions except the `allocate()` and `deallocate()`.
        template <class Allocator, class GrowthPolicy>
        class block_storage_allocator : Allocator
        {
        public:
            using embedded_storage = std::false_type;
            using arg_type         = block_storage_args_t<Allocator>;

            //=== constructors/destructors ===//
            explicit block_storage_allocator(arg_type) noexcept {}

            ~block_storage_new() noexcept
            {
                delete_block(std::move(block_));
            }

            block_storage_allocator(const block_storage_allocator&) = delete;
            block_storage_allocator& operator=(const block_storage_allocator&) = delete;

            template <typename T>
            static void swap(block_storage_allocator& lhs, block_view<T>& lhs_constructed,
                             block_storage_allocator& rhs, block_view<T>& rhs_constructed) noexcept
            {
                std::swap(lhs.block_, rhs.block_);
                std::swap(lhs_constructed, rhs_constructed);
            }

            //=== reserve/shrink_to_fit ===//
            template <typename T>
            raw_pointer reserve(size_type min_additional_bytes, const block_view<T>& constructed)
            {
                auto new_size  = GrowthPolicy::growth_size(block_.size(), min_additional_bytes);
                auto new_block = array::new_block(new_size);
                return change_block(constructed, std::move(new_block));
            }

            template <typename T>
            raw_pointer shrink_to_fit(const block_view<T>& constructed)
            {
                auto byte_size = constructed.size() * sizeof(T);
                auto new_size  = GrowthPolicy::shrink_size(block_.size(), byte_size);
                auto new_block = array::new_block(new_size);
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

            arg_type arguments() const noexcept
            {
                return {};
            }

            static size_type max_size() noexcept
            {
                return memory_block::max_size();
            }

        private:
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
                    delete_block(std::move(new_block));
                    throw;
                }

                delete_block(std::move(block_));
                block_ = new_block;

                return end;
            }

            memory_block block_;
        };
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ALLOCATOR_HPP_INCLUDED
