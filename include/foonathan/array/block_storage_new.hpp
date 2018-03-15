// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED

#include <new>
#include <type_traits>

#include <foonathan/array/block_storage_arg.hpp>
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
            return {as_raw_pointer(ptr), size};
        }

        inline void delete_block(memory_block&& block)
        {
            ::operator delete[](block.begin());
        }

        /// A `BlockStorage` that uses `operator new` for memory allocations.
        ///
        /// It does not have a small buffer optimization and uses the specified growth policy.
        template <typename T, class GrowthPolicy>
        class block_storage_new
        {
        public:
            static constexpr auto max_capacity = memory_block::max_size();
            using exact_capacity               = std::true_type;

            using arg_type = block_storage_args_t<>;

            block_storage_new(arg_type) noexcept {}

            block_storage_new(const block_storage_new&) = delete;
            block_storage_new& operator=(const block_storage_new&) = delete;

            ~block_storage_new() noexcept
            {
                if (!block_.empty())
                    delete_block(std::move(block_));
            }

            void move_construct(block_storage_new&& other, T* begin_constructed, T* end_constructed)
            {
                (void)begin_constructed;
                (void)end_constructed;

                block_       = other.block_;
                other.block_ = memory_block();
            }

            void swap(T* my_begin_constructed, T* my_end_constructed, block_storage_new& other,
                      T* other_begin_constructed, T* other_end_constructed)
            {
                (void)my_begin_constructed;
                (void)my_end_constructed;
                (void)other_begin_constructed;
                (void)other_end_constructed;

                std::swap(block_, other.block_);
            }

            raw_pointer reserve(size_type min_additional, T* begin_constructed, T* end_constructed)
            {
                auto bigger =
                    new_block(GrowthPolicy::growth_size(block_.size(), min_additional * sizeof(T)));
                return move_elements(begin_constructed, end_constructed, bigger);
            }

            raw_pointer shrink_to_fit(T* begin_constructed, T* end_constructed)
            {
                auto smaller = new_block(
                    GrowthPolicy::shrink_size(block_.size(),
                                              size_type(end_constructed - begin_constructed)
                                                  * sizeof(T)));
                return move_elements(begin_constructed, end_constructed, smaller);
            }

            const memory_block& block() const noexcept
            {
                return block_;
            }

            arg_type arguments() const noexcept
            {
                return {};
            }

        private:
            raw_pointer move_elements(T* begin_constructed, T* end_constructed,
                                      const memory_block& new_block)
            {
                auto end =
                    uninitialized_destructive_move(begin_constructed, end_constructed, new_block);
                delete_block(std::move(block_));
                block_ = new_block;
                return end;
            }

            memory_block block_;
        };
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_NEW_HPP_INCLUDED
