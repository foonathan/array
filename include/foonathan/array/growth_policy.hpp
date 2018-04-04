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
            static size_type growth_size(size_type cur_size, size_type additional_needed,
                                         size_type max_size) noexcept
            {
                (void)max_size;
                return cur_size + additional_needed;
            }

            /// \returns `size_needed`, i.e. shrink to the minimum.
            static size_type shrink_size(size_type cur_size, size_type size_needed,
                                         size_type max_size) noexcept
            {
                (void)cur_size;
                (void)max_size;
                return size_needed;
            }
        };

        namespace detail
        {
            // scales by GrowthFactor
            template <unsigned GrowthFactor>
            struct whole_growth
            {
                static_assert(GrowthFactor > 1, "does not actually grow the size");

                static size_type grow(size_type cur_size) noexcept
                {
                    return GrowthFactor * cur_size;
                }
            };

            // scales by GrowthNumerator / 2, rounding down
            template <unsigned GrowthNumerator>
            struct uneven_halfs_growth
            {
                static_assert(GrowthNumerator > 2, "does not actually grow the size");

                static constexpr auto factor = GrowthNumerator / 2;

                static size_type grow(size_type cur_size) noexcept
                {
                    // we want: GrowthNumerator / 2 * cur_size
                    //        = GrowthNumerator * cur_size / 2
                    // we know: cur_size / 2 = floor(cur_size / 2) + (cur_size mod 2) * 0.5
                    // so insert that:
                    //        = GrowthNumerator * (floor(cur_size / 2) + (cur_size mod 2) * 0.5)
                    // factor out:
                    //        = GrowthNumerator * floor(cur_size / 2) + GrowthNumerator * (cur_size mod 2) * 0.5
                    // and rearrange:
                    //        = GrowthNumerator * floor(cur_size / 2) + GrowthNumerator * (cur_size mod 2) / 2
                    //        = GrowthNumerator * floor(cur_size / 2) + factor * (cur_size mod 2)

                    auto half = cur_size / 2; // floor(cur_size / 2)
                    auto rem  = cur_size % 2; // cur_size mod 2
                    return GrowthNumerator * half + factor * rem;
                }
            };

            // scales by GrowthNumerator / GrowthDenominator, properly rounding
            template <unsigned GrowthNumerator, unsigned GrowthDenominator>
            struct frac_growth
            {
                static_assert(GrowthNumerator > GrowthDenominator,
                              "does not actually grow the size");

                static constexpr auto factor = double(GrowthNumerator) / double(GrowthDenominator);

                static size_type grow(size_type cur_size) noexcept
                {
                    return size_type(factor * double(cur_size) + 0.5);
                }
            };

            template <unsigned GrowthNumerator, unsigned GrowthDenominator>
            // clang-format off
            using select_growth =
                typename std::conditional<GrowthNumerator % GrowthDenominator == 0,
                                              whole_growth<GrowthNumerator / GrowthDenominator>, // whole number
                typename std::conditional<GrowthDenominator == 2,
                                              uneven_halfs_growth<GrowthNumerator>, // n / 2
                                        // else:
                                              frac_growth<GrowthNumerator, GrowthDenominator> // n / d
                >::type>::type;
            // clang-format on
        } // namespace detail

        /// A growth policy where it grows by the specified factor `GrowthNumerator / GrowthDenominator`.
        template <unsigned GrowthNumerator, unsigned GrowthDenominator = 1>
        class factor_growth
        {
            using growth = detail::select_growth<GrowthNumerator, GrowthDenominator>;

        public:
            /// \returns The current size multiplied by the growth factor,
            /// or the current size plus the additional one, whatever is larger.
            static size_type growth_size(size_type cur_size, size_type additional_needed,
                                         size_type max_size) noexcept
            {
                (void)max_size;
                auto needed          = cur_size + additional_needed;
                auto factored_growth = growth::grow(cur_size);
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
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_GROWTH_POLICY_HPP_INCLUDED
