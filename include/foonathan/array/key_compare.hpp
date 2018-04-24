// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_KEY_COMPARE_HPP_INCLUDED
#define FOONATHAN_ARRAY_KEY_COMPARE_HPP_INCLUDED

#include <functional>
#include <type_traits>

#if 0
/// Comparison function for a given type
template <typename Key>
struct KeyCompare
{
    /// Whether or not duplicates are allowed.
    using allow_duplicates = std::integral_constant<bool, true / false>;

    /// Compares the key with some other type.
    ///
    /// It must define a strict total ordering of the keys.
    /// `T` may be restricted to certain types, or just the key type itself.
    template <typename T>
    static key_ordering compare(const Key& key, const T& other) noexcept;
};
#endif

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
        ///
        /// If `Key` is a pointer the comparison will be done using [std::less]().
        /// Otherwise, it will first try `k.compare(t)`, then `t.compare(k)` and then `k < t`.
        ///
        /// It may or may not allow duplicates depending on the parameter.
        template <typename Key, bool AllowDuplicates>
        struct key_compare_default
        {
            using allow_duplicates = std::integral_constant<bool, AllowDuplicates>;

            template <typename T>
            static auto compare(const Key& k, const T& t) noexcept
                -> decltype(comp_detail::compare_impl(comp_detail::pointer{}, k, t))
            {
                return comp_detail::compare_impl(comp_detail::pointer{}, k, t);
            }
        };

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
