// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED

#include <new>
#include <type_traits>

#include <foonathan/array/block_storage.hpp>
#include <foonathan/array/growth_policy.hpp>

namespace foonathan
{
    namespace array
    {
        /// \effects Allocates a memory block of the given size using `::operator new[]`.
        /// \returns The allocated memory block.
        /// \throws [std::bad_alloc]() if the allocation fails.
        inline memory_block new_block(size_type size)
        {
            auto ptr = ::operator new[](size);
            return {as_raw_pointer(ptr), size};
        }

        /// \effects Deallocates a memory block using `::operator delete[]`.
        inline void delete_block(memory_block&& block) noexcept
        {
            if (!block.empty())
                ::operator delete[](block.begin());
        }

        /// A `BlockStorage` that uses `operator new` for memory allocations.
        ///
        /// It does not have a small buffer optimization and uses the specified growth policy.
        template <class GrowthPolicy>
        class block_storage_new
        {
        public:
            using embedded_storage = std::false_type;
            using arg_type         = block_storage_args_t<>;

            //=== constructors/destructors ===//
            explicit block_storage_new(arg_type) noexcept {}

            ~block_storage_new() noexcept
            {
                delete_block(std::move(block_));
            }

            block_storage_new(const block_storage_new&) = delete;
            block_storage_new& operator=(const block_storage_new&) = delete;

            template <typename T>
            static void swap(block_storage_new& lhs, block_view<T>& lhs_constructed,
                             block_storage_new& rhs, block_view<T>& rhs_constructed) noexcept
            {
                std::swap(lhs.block_, rhs.block_);
                std::swap(lhs_constructed, rhs_constructed);
            }

            //=== reserve/shrink_to_fit ===//
            template <typename T>
            raw_pointer reserve(size_type min_additional, const block_view<T>& constructed)
            {
                auto min_additional_bytes = min_additional * sizeof(T);
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
            const memory_block& block() const noexcept
            {
                return block_;
            }

            arg_type arguments() const noexcept
            {
                return {};
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
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
