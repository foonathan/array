// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_KEY_COMPARE_HPP_INCLUDED
#define FOONATHAN_ARRAY_KEY_COMPARE_HPP_INCLUDED

#include <functional>
#include <type_traits>

#include <foonathan/array/array_view.hpp>

namespace foonathan
{
    namespace array
    {
        /// The ordering of a key in relation to some other value.
        enum class key_ordering
        {
            less,       //< Other value is less than the key, i.e. sorted before it.
            equivalent, //< Other value is equivalent to the key, i.e. a duplicate.
            greater,    //< Other value is greater than key, i.e. sorted after it.
        };

        namespace comp_detail
        {
            struct op_less_than
            {
            };
            struct other_compare : op_less_than
            {
            };
            struct key_compare : other_compare
            {
            };
            struct pointer : key_compare
            {
            };

            template <typename T, typename U>
            auto compare_impl(pointer, T* k, U* t)
                -> decltype(std::less<T*>{}(t, k), key_ordering::less)
            {
                if (std::less<T*>{}(t, k))
                    return key_ordering::less;
                else if (std::less<T*>{}(k, t))
                    return key_ordering::greater;
                else
                    return key_ordering::equivalent;
            }

            template <typename Key, typename T>
            auto compare_impl(key_compare, const Key& k, const T& t)
                -> decltype(k.compare(t) < 0, key_ordering::less)
            {
                auto result = k.compare(t);
                if (result < 0)
                    return key_ordering::less;
                else if (result > 0)
                    return key_ordering::greater;
                else
                    return key_ordering::equivalent;
            }

            template <typename Key, typename T>
            auto compare_impl(other_compare, const Key& k, const T& t)
                -> decltype(t.compare(k) < 0, key_ordering::less)
            {
                auto result = t.compare(k);
                if (result < 0)
                    return key_ordering::greater;
                else if (result > 0)
                    return key_ordering::less;
                else
                    return key_ordering::equivalent;
            }

            template <typename Key, typename T>
            auto compare_impl(op_less_than, const Key& k, const T& t)
                -> decltype(k < t, t < k, key_ordering::less)
            {
                if (k < t)
                    return key_ordering::less;
                else if (t < k)
                    return key_ordering::greater;
                else
                    return key_ordering::equivalent;
            }
        } // namespace comp_detail

        /// The default comparison of keys sorting them in increasing order.
        struct key_compare_default
        {
            /// Helper struct to allow customization of the default comparison for specific keys.
            ///
            /// This is required because `key_compare_default` itself shouldn't be a template,
            /// but it should still be possible to override the default comparison.
            ///
            /// A specialization for a user-defined type must give it the same interface as required by the `KeyCompare` concept.
            ///
            template <typename Key>
            struct customize_for
            {
                /// The default default comparison.
                ///
                /// If `Key` is a pointer the comparison will be done using [std::less]().
                /// Otherwise, it will first try `k.compare(t)`, then `t.compare(k)` and then `k < t`.
                template <typename TransparentKey>
                static auto compare(const Key& k, const TransparentKey& t) noexcept
                    -> decltype(comp_detail::compare_impl(comp_detail::pointer{}, k, t))
                {
                    return comp_detail::compare_impl(comp_detail::pointer{}, k, t);
                }
            };

            template <typename Key, typename TransparentKey>
            static auto compare(const Key& k, const TransparentKey& t) noexcept
                -> decltype(customize_for<Key>::compare(k, t))
            {
                return customize_for<Key>::compare(k, t);
            }
        };

        /// A lightweight view into a sorted array.
        ///
        /// This is an [array::array_view]() where the elements are sorted according to `Compare`.
        ///
        /// \notes If you have a non sorted array, use [array::array_view]() or [array::block_view]() instead.
        /// \notes Inheritance is only used to easily inherit all functionality as well as allow conversion without a user-defined conversion.
        /// Slicing is permitted and works, but the type isn't meant to be used polymorphically.
        /// \notes Write an implicit conversion operator for containers that have contiguous storage with ordering,
        /// and specialize the [array::block_traits]() if it does not provide a `value_type` typedef.
        template <typename T, class Compare = key_compare_default>
        class sorted_view : public array_view<T>
        {
        public:
            /// \effects Creates an empty view.
            constexpr sorted_view() noexcept = default;

            /// \effects Creates a view on the [array::memory_block]().
            /// \requires The [array::memory_block]() must contain objects of type `T`.
            explicit constexpr sorted_view(const memory_block& block) noexcept
            : array_view<T>(block)
            {
            }

            /// \effects Creates a view on the range `[data, data + size)`.
            constexpr sorted_view(T* data, size_type size) : array_view<T>(data, size) {}

            /// \effects Creates a view on the range `[begin, end)`.
            /// \notes This constructor only participates in overload resolution if `ContIter` is a contiguous iterator,
            /// and the value type is the same.
            template <typename ContIter,
                      typename = typename std::enable_if<
                          std::is_same<T, contiguous_iterator_value_type<ContIter>>::value>::type>
            constexpr sorted_view(ContIter begin, ContIter end) : array_view<T>(begin, end)
            {
            }

            /// \effects Creates a view on the given array.
            template <std::size_t N>
            explicit constexpr sorted_view(T (&array)[N]) noexcept : array_view<T>(array)
            {
            }

            /// \effects Converts a view to non-const to a view to const.
            /// \notes This constructor only participates in overload resolution, if `const U` is the same as `T`,
            /// i.e. `T` is `const` and `U` is the non-`const` version of it.
            template <typename U,
                      typename = typename std::enable_if<std::is_same<const U, T>::value>::type>
            constexpr sorted_view(const sorted_view<U>& other) noexcept : array_view<T>(other)
            {
            }

            /// \effects Creates a view to the block.
            /// \requires The block is actually sorted.
            explicit constexpr sorted_view(const block_view<T>& block) noexcept
            : array_view<T>(block)
            {
            }

            //=== sorted access ===//
            /// \returns A reference to the minimal element.
            /// \requires `!empty()`
            constexpr T& min() const noexcept
            {
                return this->front();
            }

            /// \returns A reference to the maximal element.
            /// \requires `!empty()`
            constexpr T& max() const noexcept
            {
                return this->back();
            }
        };

        /// \returns The sorted view viewing the given block.
        /// \notes This gives the block ordering which may not be there.
        template <class Compare = key_compare_default, typename T>
        constexpr sorted_view<T, Compare> make_sorted_view(const block_view<T>& block) noexcept
        {
            return sorted_view<T, Compare>(block);
        }

        /// \returns A view to the range `[data, data + size)`.
        template <class Compare = key_compare_default, typename T>
        constexpr sorted_view<T> make_sorted_view(T* data, size_type size) noexcept
        {
            return sorted_view<T, Compare>(data, size);
        }

        /// \returns A view to the range `[begin, end)`.
        /// \notes This function does not participate in overload resolution, unless they are contiguous iterators.
        template <class Compare = key_compare_default, typename ContIter>
        constexpr auto make_sorted_view(ContIter begin, ContIter end) noexcept
            -> sorted_view<contiguous_iterator_value_type<ContIter>>
        {
            return {begin, end};
        }

        /// \returns A view to the array.
        template <class Compare = key_compare_default, typename T, std::size_t N>
        constexpr sorted_view<T, Compare> make_sorted_view(T (&array)[N]) noexcept
        {
            return sorted_view<T, Compare>(array);
        }

        /// \returns An iterator pointing to the first element that is greater or equal to the given key.
        /// \requires The sequence is sorted according to the key comparison.
        template <class Compare, typename ForwardIt, typename Key>
        ForwardIt lower_bound(ForwardIt begin, ForwardIt end, const Key& key)
        {
            auto length = std::distance(begin, end);
            while (length != 0)
            {
                auto half_length = length / 2;
                auto mid         = std::next(begin, half_length);
                if (Compare::compare(*mid, key) == key_ordering::less)
                {
                    begin = std::next(mid);
                    length -= half_length + 1;
                }
                else
                    length = half_length;
            }
            return begin;
        }

        /// \returns An iterator pointing to the first element that is greater to the given key.
        /// \requires The sequence is sorted according to the key comparison.
        template <class Compare, typename ForwardIt, typename Key>
        ForwardIt upper_bound(ForwardIt begin, ForwardIt end, const Key& key)
        {
            auto length = std::distance(begin, end);
            while (length != 0)
            {
                auto half_length = length / 2;
                auto mid         = std::next(begin, half_length);
                if (Compare::compare(*mid, key) == key_ordering::greater)
                    length = half_length;
                else
                {
                    begin = std::next(mid);
                    length -= half_length + 1;
                }
            }
            return begin;
        }

        /// A pair of two iterators.
        template <typename Iter>
        struct iter_pair
        {
            Iter first, second;

            Iter begin() const noexcept
            {
                return first;
            }

            Iter end() const noexcept
            {
                return second;
            }

            bool empty() const noexcept
            {
                return first == second;
            }

            /// \returns A view into the range denoted by the pair.
            /// \requires The iterators are contiguous.
            /// \param Iter2
            /// \exclude
            template <typename Iter2 = Iter>
            auto view() const noexcept -> block_view<contiguous_iterator_value_type<Iter2>>
            {
                return make_block_view(first, second);
            }
        };

        /// \returns A pair of two iterators where the first one is the result of [array::lower_bound]()
        /// and the second one the result of [array::upper_bound]().
        /// \requires The sequence is sorted according to the key comparison.
        template <class Compare, typename ForwardIt, typename Key>
        iter_pair<ForwardIt> equal_range(ForwardIt begin, ForwardIt end, const Key& key)
        {
            auto length = std::distance(begin, end);
            while (length != 0)
            {
                auto half_length = length / 2;
                auto mid         = std::next(begin, half_length);

                auto compare_result = Compare::compare(*mid, key);
                if (compare_result == key_ordering::less)
                {
                    begin = std::next(mid);
                    length -= half_length + 1;
                }
                else if (compare_result == key_ordering::greater)
                {
                    end    = mid;
                    length = half_length;
                }
                else
                {
                    auto lower = lower_bound<Compare>(begin, mid, key);
                    auto upper = upper_bound<Compare>(std::next(mid), end, key);
                    return {lower, upper};
                }
            }
            return {begin, end};
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_KEY_COMPARE_HPP_INCLUDED
