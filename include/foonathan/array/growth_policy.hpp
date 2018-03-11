// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_GROWTH_POLICY_HPP_INCLUDED
#define FOONATHAN_ARRAY_GROWTH_POLICY_HPP_INCLUDED

#include <type_traits>

#include <foonathan/array/memory_block.hpp>

namespace foonathan
{
    namespace array
    {
        /// A growth policy where it does not grow beyond what's necessary.
        /// \notes Using it without manually calling `reserve()` leads to bad performance.
        struct no_extra_growth
        {
            /// \returns The new size of the block, that is `cur_size + additional_needed`.
            static constexpr size_type growth_size(size_type cur_size,
                                                   size_type additional_needed) noexcept
            {
                return cur_size + additional_needed;
            }

            /// \returns `size_needed`, i.e. shrink to the minimum.
            static constexpr size_type shrink_size(size_type cur_size,
                                                   size_type size_needed) noexcept
            {
                return (void)cur_size, size_needed;
            }
        };

        /// A growth policy where it grows by the specified factor.
        template <unsigned GrowthNumerator, unsigned GrowthDenominator = 1>
        class factor_growth
        {
            using factor_type = typename std::conditional<GrowthNumerator % GrowthDenominator == 0,
                                                          size_type, float>::type;

        public:
            static constexpr auto growth_factor = factor_type(GrowthNumerator) / GrowthDenominator;

            /// \returns The current size multiplied by the growth factor,
            /// or the current size plus the additional one, whatever is larger.
            static size_type growth_size(size_type cur_size, size_type additional_needed) noexcept
            {
                auto needed          = cur_size + additional_needed;
                auto factored_growth = size_type(growth_factor * cur_size);
                return factored_growth > needed ? factored_growth : needed;
            }

            /// \returns `size_needed`, i.e. shrink to the minimum.
            static size_type shrink_size(size_type cur_size, size_type size_needed) noexcept
            {
                (void)cur_size;
                return size_needed;
            }
        };

        /// The default growth policy.
        using default_growth = factor_growth<2>;
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_GROWTH_POLICY_HPP_INCLUDED
