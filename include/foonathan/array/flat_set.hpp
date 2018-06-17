// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_FLAT_SET_HPP_INCLUDED
#define FOONATHAN_ARRAY_FLAT_SET_HPP_INCLUDED

#include <foonathan/array/array.hpp>
#include <foonathan/array/key_compare.hpp>

namespace foonathan
{
    namespace array
    {
        /// A pair of a key with a value.
        ///
        /// Use this in [array::flat_set]() if you want to have an [array::flat_map]() where the values are stored together with the keys.
        /// Only the `key` is being compared, not the value.
        template <typename Key, typename Value>
        struct key_value_pair
        {
            Key           key;
            mutable Value value;

            /// \effects Creates it by constructing the key from the given key and the value from the arguments.
            template <typename TransparentKey, typename... ValueArgs>
            explicit key_value_pair(TransparentKey&& key, ValueArgs&&... args)
            : key(std::forward<TransparentKey>(key)), value(std::forward<ValueArgs>(args)...)
            {
            }
        };

        /// Specialization of [array::key_compare_default]() for the [array::key_value_pair]().
        ///
        /// It will only compare the key.
        template <typename Key, typename Value>
        struct key_compare_default::customize_for<key_value_pair<Key, Value>>
        {
            template <typename T>
            static auto compare(const key_value_pair<Key, Value>& pair, const T& t) noexcept
                -> decltype(key_compare_default::compare(pair.key, t))
            {
                return key_compare_default::compare(pair.key, t);
            }
        };

        namespace detail
        {
            template <std::size_t I, typename KeyValue>
            struct get_key_value;

            template <typename Key, typename Value>
            struct get_key_value<0, key_value_pair<Key, Value>>
            {
                using type = const Key;

                static constexpr type& get(const key_value_pair<Key, Value>& key_value)
                {
                    return key_value.key;
                }
            };

            template <typename Key, typename Value>
            struct get_key_value<1, key_value_pair<Key, Value>>
            {
                using type = Value;

                static constexpr type& get(const key_value_pair<Key, Value>& key_value)
                {
                    return key_value.value;
                }
            };
        } // namespace detail

        template <std::size_t I, typename Key, typename Value>
        constexpr typename detail::get_key_value<I, key_value_pair<Key, Value>>::type& get(
            const key_value_pair<Key, Value>& key_value)
        {
            return detail::get_key_value<I, key_value_pair<Key, Value>>::get(key_value);
        }

        namespace detail
        {
            template <typename Iterator>
            class insert_result
            {
            public:
                /// \returns An iterator to the element with the given key.
                Iterator iter() const noexcept
                {
                    return iter_;
                }

                /// \returns Whether or not the key was already present in the set.
                bool was_duplicate() const noexcept
                {
                    return kind_ != insert;
                }

                /// \returns Whether or not the key was inserted into the set.
                bool was_inserted() const noexcept
                {
                    return kind_ == insert || kind_ == insert_dup;
                }

                /// \returns Whether or not the key was being replaced.
                bool was_replaced() const noexcept
                {
                    return kind_ == replace;
                }

            private:
                enum kind
                {
                    insert,
                    insert_dup,
                    replace,
                    nothing,
                };

                insert_result(Iterator iter, kind k) : iter_(iter), kind_(k) {}

                Iterator iter_;
                kind     kind_;

                template <typename Iter>
                friend insert_result<Iter> result_inserted(Iter iter);
                template <typename Iter>
                friend insert_result<Iter> result_inserted_duplicate(Iter iter);
                template <typename Iter>
                friend insert_result<Iter> result_replaced(Iter iter);
                template <typename Iter>
                friend insert_result<Iter> result_nothing(Iter iter);
            };

            template <typename Iterator>
            insert_result<Iterator> result_inserted(Iterator iter)
            {
                return {iter, insert_result<Iterator>::insert};
            }
            template <typename Iterator>
            insert_result<Iterator> result_inserted_duplicate(Iterator iter)
            {
                return {iter, insert_result<Iterator>::insert_dup};
            }
            template <typename Iterator>
            insert_result<Iterator> result_replaced(Iterator iter)
            {
                return {iter, insert_result<Iterator>::replace};
            }
            template <typename Iterator>
            insert_result<Iterator> result_nothing(Iterator iter)
            {
                return {iter, insert_result<Iterator>::nothing};
            }

            template <typename T, typename Arg>
            auto replace_value(T& obj, Arg&& arg) -> decltype(obj = std::forward<Arg>(arg))
            {
                return obj = std::forward<Arg>(arg);
            }
            template <typename T, typename... Args>
            void replace_value(T& obj, Args&&... args)
            {
                obj = T(std::forward<Args>(args)...);
            }
        } // namespace detail

        /// A sorted set of elements.
        ///
        /// It is similar to [std::set]() or [std::multiset]() — depending on `AllowDuplicates`,
        /// but uses a sorted [array::array]() with the given `BlockStorage` internally.
        ///
        /// `Compare` must be a `KeyCompare` type, not something like [std::less]().
        ///
        /// \notes When you have a `flat_set<key_value_pair<Key, Value>>`,
        /// you have something similar to [array::flat_map]() but where the keys and values are stored together.
        template <typename Key, typename Compare = key_compare_default,
                  class BlockStorage = block_storage_default, bool AllowDuplicates = false>
        class flat_set
        {
            class iterator_tag
            {
                constexpr iterator_tag() = default;

                friend flat_set;
            };

            // TODO: exception safety when sort throws

        public:
            using key_type   = Key;
            using value_type = key_type;

            using key_compare   = Compare;
            using value_compare = key_compare;

            using is_multiset = std::integral_constant<bool, AllowDuplicates>;

            using block_storage = BlockStorage;

            using iterator       = pointer_iterator<iterator_tag, const Key>;
            using const_iterator = iterator;

            //=== constructors/destructors ===//
            /// Default constructor.
            /// \effects Creates a set without any elements.
            /// The block storage is initialized with default constructed arguments.
            flat_set() = default;

            /// \effects Creates a set without any elements.
            /// The block storage is initialized with the given arguments.
            explicit flat_set(argument_type<BlockStorage> arg) noexcept : array_(arg) {}

            /// \effects Creates a set containing the elements of the view.
            /// The block storage is initialized with the given arguments.
            explicit flat_set(input_view<Key, BlockStorage>&& input,
                              argument_type<BlockStorage>     arg = {})
            : array_(arg)
            {
                assign(std::move(input));
            }

            /// \effects Same as `assign(std::move(view))`.
            flat_set& operator=(input_view<Key, BlockStorage>&& view)
            {
                assign(std::move(view));
                return *this;
            }

            /// Swap.
            friend void swap(flat_set& lhs, flat_set& rhs) noexcept(
                block_storage_nothrow_move<BlockStorage, Key>::value)
            {
                swap(lhs.array_, rhs.array_);
            }

            //=== access ===//
            /// \returns A sorted view to the elements.
            operator sorted_view<const Key, Compare>() const noexcept
            {
                return sorted_view<const Key, Compare>(array_);
            }

            /// \returns An input view to the elements.
            operator input_view<Key, BlockStorage>() && noexcept
            {
                return std::move(array_).operator input_view<Key, BlockStorage>();
            }

            const_iterator begin() const noexcept
            {
                return cbegin();
            }
            const_iterator cbegin() const noexcept
            {
                return convert_iterator(array_.cbegin());
            }

            const_iterator end() const noexcept
            {
                return cend();
            }
            const_iterator cend() const noexcept
            {
                return convert_iterator(array_.cend());
            }

            /// \returns A reference to the minimal element.
            const Key& min() const noexcept
            {
                return array_.front();
            }

            /// \returns A reference to the maximal element.
            const Key& max() const noexcept
            {
                return array_.back();
            }

            //=== capacity ===//
            /// \returns Whether or not the set is empty.
            bool empty() const noexcept
            {
                return array_.empty();
            }

            /// \returns The number of elements in the set.
            size_type size() const noexcept
            {
                return array_.size();
            }

            /// \returns The number of elements the set can contain without reserving new memory.
            size_type capacity() const noexcept
            {
                return array_.capacity();
            }

            /// \returns The maximum number of elements as determined by the block storage.
            size_type max_size() const noexcept
            {
                return array_.max_size();
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity` if that isn't the case already.
            void reserve(size_type new_capacity)
            {
                array_.reserve(new_capacity);
            }

            /// \effects Non-binding request to make the capacity as small as necessary.
            void shrink_to_fit()
            {
                array_.shrink_to_fit();
            }

            //=== modifiers ===//
            /// The result of an insert operation.
            using insert_result = detail::insert_result<iterator>;

            /// \effects Does a lookup of the given key.
            /// If the key is not already in the set or the set is a multiset,
            /// inserts the key constructed by forwarding `key` followed by `args`.
            /// Otherwise, does nothing.
            /// \returns The result of the insert operation.
            template <typename TransparentKey, typename... Args>
            insert_result emplace(TransparentKey&& key, Args&&... args)
            {
                auto range = equal_range(key);
                if (AllowDuplicates || range.empty())
                {
                    // we either don't care about duplicates or the key is not in the map
                    auto iter = array_.emplace(convert_iterator(range.end()),
                                               std::forward<TransparentKey>(key),
                                               std::forward<Args>(args)...);
                    if (range.empty())
                        return detail::result_inserted(convert_iterator(iter));
                    else
                        return detail::result_inserted_duplicate(convert_iterator(iter));
                }
                else
                {
                    // we don't allow duplicates and the key is already in the map
                    assert(std::next(range.begin()) == range.end());
                    return detail::result_nothing(range.begin());
                }
            }

            /// \effects Does a lookup of the given key.
            /// If the key is not already in the set, inserts the key constructed by forwarding `key` followed by `args`.
            /// Otherwise, does nothing.
            /// \returns The result of the insert operation.
            /// \requires The set is a multiset.
            /// \notes This function is the same as `emplace()` on a non-multiset.
            template <typename TransparentKey, typename... Args>
            insert_result emplace_unique(TransparentKey&& key, Args&&... args)
            {
                static_assert(sizeof(TransparentKey) == sizeof(TransparentKey) && AllowDuplicates,
                              "emplace_unique doesn't make sense on non-multiset");
                auto range = equal_range(key);
                if (range.empty())
                {
                    // key not in the map
                    auto iter = array_.emplace(convert_iterator(range.end()),
                                               std::forward<TransparentKey>(key),
                                               std::forward<Args>(args)...);
                    return detail::result_inserted(convert_iterator(iter));
                }
                else
                {
                    // we don't allow duplicates and the key is already in the map
                    return detail::result_nothing(range.begin());
                }
            }

            /// \effects Does a lookup of the given key.
            /// If the key is not already in the set, inserts the key constructed by forwarding `key` followed by `args`.
            /// Otherwise, replaces the key with the key constructed by forwarding `key` followed by `args`.
            /// \returns The result of the insert operation.
            /// \requires The set is not a multiset and the new key must be inserted in the same position as the old key.
            /// The latter is guaranteed if the transparent key argument identifies the part of the key that is actually being compared,
            /// and the other arguments are ignored.
            template <typename TransparentKey, typename... Args>
            insert_result emplace_or_replace(TransparentKey&& key, Args&&... args)
            {
                static_assert(sizeof(TransparentKey) == sizeof(TransparentKey) && !AllowDuplicates,
                              "emplace_or_replace doesn't make sense on multiset");
                auto result =
                    emplace(std::forward<TransparentKey>(key), std::forward<Args>(args)...);
                if (result.was_duplicate())
                {
                    // can re-use arguments, they are not moved from if no insert took place

                    // this const cast does not lead to UB because no element was const to begin with
                    // we're also requiring that it doesn't change the sorting oder
                    detail::replace_value(const_cast<Key&>(*result.iter()),
                                          std::forward<TransparentKey>(key),
                                          std::forward<Args>(args)...);
                    return detail::result_replaced(result.iter());
                }
                else
                    return detail::result_inserted(result.iter());
            }

            /// \effects Same as the emplace variant being called with `FWD(k)`.
            /// \notes This function does not participate in overload resolution if `Key` is not constructible from `K`.
            /// \group insert
            /// \param 1
            /// \exclude
            template <typename K,
                      typename = typename std::enable_if<std::is_convertible<K, Key>::value>::type>
            insert_result insert_unique(K&& k)
            {
                return emplace_unique(std::forward<K>(k));
            }
            /// \group insert
            /// \param 1
            /// \exclude
            template <typename K,
                      typename = typename std::enable_if<std::is_convertible<K, Key>::value>::type>
            insert_result insert(K&& k)
            {
                return emplace(std::forward<K>(k));
            }
            /// \group insert
            /// \param 1
            /// \exclude
            template <typename K,
                      typename = typename std::enable_if<std::is_convertible<K, Key>::value>::type>
            insert_result insert_or_replace(K&& k)
            {
                return emplace_or_replace(std::forward<K>(k));
            }

            /// \effects Same as `insert_range(view.begin(), view.end())`.
            void insert(const block_view<const Key>& view)
            {
                insert_range(view.begin(), view.end());
            }

            /// \effects Inserts all elements in the range `[begin, end)` as-if calling `insert()`.
            template <typename InputIt>
            void insert_range(InputIt begin, InputIt end)
            {
                insert_range_impl(typename std::iterator_traits<InputIt>::iterator_category{},
                                  begin, end);
            }

            /// \effects Destroys and removes all elements.
            void clear() noexcept
            {
                array_.clear();
            }

            /// \effects Destroys and removes the element at the given position.
            /// \returns An iterator after the element that was removed.
            iterator erase(iterator pos) noexcept(std::is_nothrow_move_assignable<Key>::value)
            {
                return convert_iterator(array_.erase(convert_iterator(pos)));
            }

            /// \effects Destroys and removes all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            iterator erase_range(iterator begin,
                                 iterator end) noexcept(std::is_nothrow_move_assignable<Key>::value)
            {
                return convert_iterator(
                    array_.erase_range(convert_iterator(begin), convert_iterator(end)));
            }

            /// \effects Destroys and removes all occurrences of `key`.
            /// \returns The number of elements that were removed, if it doesn't allow duplicates,
            /// whether or not any were removed otherwise.
            template <typename TransparentKey>
            auto erase_all(const TransparentKey& key) noexcept(
                std::is_nothrow_move_assignable<Key>::value) ->
                typename std::conditional<AllowDuplicates, size_type, bool>::type
            {
                auto range = equal_range(key);

                using result_type =
                    typename std::conditional<AllowDuplicates, size_type, bool>::type;
                auto count = result_type(range.end() - range.begin());

                erase_range(range.begin(), range.end());

                return count;
            }

            /// \effects Conceptually the same as `*this = flat_set<Key>(input)`.
            void assign(input_view<Key, BlockStorage>&& input)
            {
                if (input.will_steal_memory())
                {
                    // steal memory, then sort + unique is probably going to be faster
                    // than the individual insert done below

                    array_.assign(std::move(input));

                    std::sort(array_.begin(), array_.end(), [&](const Key& lhs, const Key& rhs) {
                        return Compare::compare(lhs, rhs) == key_ordering::less;
                    });

                    if (!AllowDuplicates)
                    {
                        auto new_end = std::unique(array_.begin(), array_.end(),
                                                   [&](const Key& lhs, const Key& rhs) {
                                                       return Compare::compare(lhs, rhs)
                                                              == key_ordering::equivalent;
                                                   });
                        array_.erase_range(new_end, array_.end());
                    }
                }
                else
                {
                    array_.clear();
                    array_.reserve(input.size());

                    // insert all elements individually
                    for (auto& element : input.view())
                    {
                        if (input.will_copy())
                            insert(element);
                        else
                        {
                            // safe, according to precondition of input view,
                            // we're allowed to move them
                            auto& non_const = const_cast<Key&>(element);
                            insert(std::move(non_const));
                        }
                    }
                }
            }

            /// \effects Conceptually the same as `flat_set<Key> s; s.insert_range(begin, end); *this = std::move(s);`
            template <typename InputIt>
            void assign_range(InputIt begin, InputIt end)
            {
                array_.assign_range(begin, end);
                std::sort(array_.begin(), array_.end(), [&](const Key& lhs, const Key& rhs) {
                    return Compare::compare(lhs, rhs) == key_ordering::less;
                });
            }

            //=== lookup ===//
            /// \returns Whether or not the key is contained in the set.
            template <typename TransparentKey>
            bool contains(const TransparentKey& key) const noexcept
            {
                return find(key) != end();
            }

            /// \returns The key that is considered equal to the given transparent key.
            /// \requires The key must be stored in the map.
            template <typename TransparentKey>
            const Key& lookup(const TransparentKey& key) const noexcept
            {
                auto iter = find(key);
                assert(iter != end());
                return *iter;
            }

            /// \returns A pointer to the key that is considered equal to the given transparent key,
            /// or `nullptr`, if there was none.
            template <typename TransparentKey>
            const Key* try_lookup(const TransparentKey& key) const noexcept
            {
                auto iter = find(key);
                if (iter == end())
                    return nullptr;
                else
                    return &*iter;
            }

            /// \returns An iterator to the given key, or `end()` if the key is not in the set.
            template <typename TransparentKey>
            const_iterator find(const TransparentKey& key) const noexcept
            {
                auto lower = lower_bound(key);
                if (lower == end())
                    return end();
                else if (Compare::compare(*lower, key) == key_ordering::equivalent)
                    return lower;
                else
                    return end();
            }

            /// \returns The number of occurences of `key` in the set.
            /// \notes If `Compare::allow_duplicates == std::false_type` this is either `0` or `1`.
            template <typename TransparentKey>
            size_type count(const TransparentKey& key) const noexcept
            {
                auto range = equal_range(key);
                return size_type(range.end() - range.begin());
            }

            /// \returns Same as [array::lower_bound]() for the given `key`.
            template <typename TransparentKey>
            const_iterator lower_bound(const TransparentKey& key) const noexcept
            {
                return foonathan::array::lower_bound<Compare>(begin(), end(), key);
            }

            /// \returns Same as [array::upper_bound]() for the given `key`.
            template <typename TransparentKey>
            const_iterator upper_bound(const TransparentKey& key) const noexcept
            {
                return foonathan::array::upper_bound<Compare>(begin(), end(), key);
            }

            /// \returns Same as [array::equal_range]() for the given `key`.
            template <typename TransparentKey>
            iter_pair<const_iterator> equal_range(const TransparentKey& key) const noexcept
            {
                return foonathan::array::equal_range<Compare>(begin(), end(), key);
            }

        private:
            static iterator convert_iterator(
                typename array<Key, BlockStorage>::const_iterator iter) noexcept
            {
                auto ptr = iterator_to_pointer(iter);
                return iterator(iterator_tag{}, ptr);
            }

            static typename array<Key, BlockStorage>::const_iterator convert_iterator(
                iterator iter) noexcept
            {
                auto ptr = iterator_to_pointer(iter);
                return pointer_to_iterator<typename array<Key, BlockStorage>::const_iterator>(ptr);
            }

            template <typename InputIt>
            void insert_range_impl(std::input_iterator_tag, InputIt begin, InputIt end)
            {
                for (auto cur = begin; cur != end; ++cur)
                    insert(*cur);
            }
            template <typename ForwardIt>
            void insert_range_impl(std::forward_iterator_tag, ForwardIt begin, ForwardIt end)
            {
                auto size = std::distance(begin, end);
                array_.reserve(array_.size() + size_type(size));

                insert_range_impl(std::input_iterator_tag{}, begin, end);
            }

            array<Key, BlockStorage> array_;
        };

        /// Convenience typedef for an [array::flat_set]() that allows duplicates.
        template <typename Key, typename Compare = key_compare_default,
                  class BlockStorage = block_storage_default>
        using flat_multiset = flat_set<Key, Compare, BlockStorage, true>;
    } // namespace array
} // namespace foonathan

namespace std
{
    template <typename Key, typename Value>
    struct tuple_size<foonathan::array::key_value_pair<Key, Value>>
    : std::integral_constant<std::size_t, 2u>
    {
    };

    template <std::size_t I, typename Key, typename Value>
    struct tuple_element<I, foonathan::array::key_value_pair<Key, Value>>
    {
        using type = typename foonathan::array::detail::get_key_value<
            I, foonathan::array::key_value_pair<Key, Value>>::type;
    };
} // namespace std

#endif // FOONATHAN_ARRAY_FLAT_SET_HPP_INCLUDED
