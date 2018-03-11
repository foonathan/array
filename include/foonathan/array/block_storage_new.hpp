// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED

#include <new>
#include <type_traits>

#include <foonathan/array/growth_policy.hpp>
#include <foonathan/array/memory_block.hpp>
#include <foonathan/array/raw_storage.hpp>

namespace foonathan
{
    namespace array
    {
        inline memory_block new_block(size_type size)
        {
            auto ptr = ::operator new[](size);
            return {from_pointer(ptr), size};
        }

        inline void delete_block(memory_block&& block)
        {
            ::operator delete[](block.memory);
        }

        /// A `BlockStorage` that uses `operator new` for memory allocations.
        ///
        /// It does not have a small buffer optimization and uses the specified growth policy.
        template <class GrowthPolicy>
        class block_storage_new
        {
        public:
            static constexpr auto max_capacity = memory_block::max_size();
            using exact_capacity               = std::true_type;

            block_storage_new() noexcept = default;

            explicit block_storage_new(size_type min_initial_capacity)
            : block_(new_block(min_initial_capacity))
            {
            }

            ~block_storage_new() noexcept
            {
                if (block_)
                    delete_block(std::move(block_));
            }

            block_storage_new(const block_storage_new&) = delete;
            block_storage_new& operator=(const block_storage_new&) = delete;

            template <typename T>
            T* reserve(size_type min_additional, T* begin_constructed, T* end_constructed)
            {
                auto bigger = new_block(GrowthPolicy::growth_size(block_.size, min_additional));
                return move_elements(begin_constructed, end_constructed, bigger);
            }

            template <typename T>
            T* shrink_to_fit(T* begin_constructed, T* end_constructed)
            {
                auto smaller = new_block(
                    GrowthPolicy::shrink_size(block_.size,
                                              size_type(end_constructed - begin_constructed)));
                return move_elements(begin_constructed, end_constructed, smaller);
            }

            const memory_block& block() const noexcept
            {
                return block_;
            }

        private:
            template <typename T>
            T* move_elements(T* begin_constructed, T* end_constructed,
                             const memory_block& new_block)
            {
                auto first_new =
                    uninitialized_destructive_move(begin_constructed, end_constructed, new_block);
                delete_block(std::move(block_));
                block_ = new_block;
                return first_new;
            }

            memory_block block_;
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
