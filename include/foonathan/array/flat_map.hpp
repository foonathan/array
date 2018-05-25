// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_FLAT_MAP_HPP_INCLUDED
#define FOONATHAN_ARRAY_FLAT_MAP_HPP_INCLUDED

#include <foonathan/array/array.hpp>
#include <foonathan/array/flat_set.hpp>

namespace foonathan
{
    namespace array
    {
        /// The key value pair of a map.
        ///
        /// This is actually a proxy that behaves like `T&` as the map doesn't actually store a pair.
        template <typename Key, typename Value>
        struct key_value_ref
        {
            const Key& key;
            Value&     value;

            constexpr key_value_ref(const Key& key, Value& value) : key(key), value(value) {}

            key_value_ref(const key_value_ref&) = default;
            ~key_value_ref()                    = default;

            key_value_ref& operator=(const key_value_ref&) = delete;
        };

        namespace detail
        {
            template <std::size_t I, typename KeyValue>
            struct get_key_value;

            template <typename Key, typename Value>
            struct get_key_value<0, key_value_ref<Key, Value>>
            {
                using type = const Key;

                static constexpr type& get(const key_value_ref<Key, Value>& key_value)
                {
                    return key_value.key;
                }
            };

            template <typename Key, typename Value>
            struct get_key_value<1, key_value_ref<Key, Value>>
            {
                using type = Value;

                static constexpr type& get(const key_value_ref<Key, Value>& key_value)
                {
                    return key_value.value;
                }
            };
        } // namespace detail

        template <std::size_t I, typename Key, typename Value>
        constexpr typename detail::get_key_value<I, key_value_ref<Key, Value>>::type& get(
            const key_value_ref<Key, Value>& key_value)
        {
            return detail::get_key_value<I, key_value_ref<Key, Value>>::get(key_value);
        }

        namespace detail
        {
            template <typename Key, typename Value>
            class flat_map_iterator
            {
                static_assert(std::is_trivially_copyable<key_value_ref<Key, Value>>::value,
                              "pair must be trivially copyable");

            public:
                struct private_key
                {
                    explicit private_key() {}
                };

                using iterator_category = std::random_access_iterator_tag;
                using value_type        = key_value_ref<Key, Value>;
                using difference_type   = std::ptrdiff_t;

                // thanks to @ubsan for this
                struct pointer
                {
                    mutable value_type pair;

                    value_type* operator->() const noexcept
                    {
                        return &pair;
                    }
                };

                using reference = value_type; // can't use a real reference

                flat_map_iterator() noexcept = default;

                explicit flat_map_iterator(private_key, const Key* key, Value* value) noexcept
                : key_(key), value_(value)
                {
                }

                flat_map_iterator(
                    const flat_map_iterator<Key, typename std::remove_const<Value>::type>&
                        non_const) noexcept
                : key_(non_const.key_), value_(non_const.value_)
                {
                }

                //=== access ===//
                reference operator*() const noexcept
                {
                    return reference(*key_, *value_);
                }

                pointer operator->() const noexcept
                {
                    return pointer{**this};
                }

                value_type operator[](difference_type offset) const noexcept
                {
                    return key_value_ref<Key, Value>(key_[offset], value_[offset]);
                }

                const Key* get_key_pointer(private_key) const noexcept
                {
                    return key_;
                }

                Value* get_value_pointer(private_key) const noexcept
                {
                    return value_;
                }

                //=== increment/decrement ===//
                flat_map_iterator& operator++() noexcept
                {
                    return *this += 1;
                }
                flat_map_iterator operator++(int) noexcept
                {
                    auto save = *this;
                    ++*this;
                    return save;
                }

                flat_map_iterator& operator--() noexcept
                {
                    return *this += -1;
                }
                flat_map_iterator operator--(int) noexcept
                {
                    auto save = *this;
                    --*this;
                    return save;
                }

                flat_map_iterator& operator+=(difference_type offset) noexcept
                {
                    key_ += offset;
                    value_ += offset;
                    return *this;
                }
                flat_map_iterator& operator-=(difference_type offset) noexcept
                {
                    return *this += -offset;
                }

                //=== addition/subtraction ===//
                friend flat_map_iterator operator+(flat_map_iterator iter,
                                                   difference_type   offset) noexcept
                {
                    return flat_map_iterator(private_key{}, iter.key_ + offset,
                                             iter.value_ + offset);
                }
                friend flat_map_iterator operator+(difference_type   offset,
                                                   flat_map_iterator iter) noexcept
                {
                    return iter + offset;
                }

                friend flat_map_iterator operator-(flat_map_iterator iter,
                                                   difference_type   offset) noexcept
                {
                    return iter + (-offset);
                }
                friend flat_map_iterator operator-(difference_type   offset,
                                                   flat_map_iterator iter) noexcept
                {
                    return iter - offset;
                }

                friend difference_type operator-(flat_map_iterator lhs, flat_map_iterator rhs)
                {
                    return lhs.key_ - rhs.key_;
                }

                //=== comparison ===//
                friend bool operator==(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ == rhs.key_;
                }
                friend bool operator!=(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ != rhs.key_;
                }

                friend bool operator<(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ < rhs.key_;
                }
                friend bool operator>(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ > rhs.key_;
                }
                friend bool operator<=(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ <= rhs.key_;
                }
                friend bool operator>=(flat_map_iterator lhs, flat_map_iterator rhs) noexcept
                {
                    return lhs.key_ >= rhs.key_;
                }

            private:
                const Key* key_;
                Value*     value_;

                template <typename, typename>
                friend class flat_map_iterator;
            };
        } // namespace detail

        /// A sorted map of keys to values.
        ///
        /// It is similar to [std::map]() or [std::multimap]() — depending on `AllowDuplicates`,
        /// but uses a [array::flat_set]() for the key storage and an [array::array]() internally, both using the given `BlockStorage`.
        /// Matching keys and values are implicitly linked by being stored at the same index.
        ///
        /// `Compare` must be a `KeyCompare` type, not something like [std::less]().
        template <typename Key, typename Value, class Compare = key_compare_default,
                  class BlockStorage = block_storage_default, bool AllowDuplicates = false>
        class flat_map
        {
            using key_storage   = flat_set<Key, Compare, BlockStorage, AllowDuplicates>;
            using value_storage = array<Value, BlockStorage>;

        public:
            using key_type   = Key;
            using value_type = Value;

            using key_compare   = Compare;
            using value_compare = key_compare;

            using is_multimap = std::integral_constant<bool, AllowDuplicates>;

            using block_storage = BlockStorage;

            using key_iterator         = typename key_storage::iterator;
            using key_const_iterator   = key_iterator;
            using value_iterator       = typename value_storage::iterator;
            using value_const_iterator = typename value_storage::const_iterator;

            using iterator       = detail::flat_map_iterator<Key, Value>;
            using const_iterator = detail::flat_map_iterator<Key, const Value>;

            //=== constructors/destructors ===//
            /// Default constructor.
            /// \effects Creates a map without any elements.
            /// The block storage is initialized with default constructed arguments.
            flat_map() = default;

            /// \effects Creates a map without any elements.
            /// The block storage is initialized with the given arguments.
            /// \notes It will be copied to initialize the two different containers.
            explicit flat_map(typename block_storage::arg_type args) noexcept
            : keys_(args), values_(std::move(args))
            {
            }

            /// Swap.
            friend void swap(flat_map& lhs, flat_map& rhs) noexcept(
                block_storage_nothrow_move<BlockStorage, Key>{})
            {
                swap(lhs.keys_, rhs.keys_);
                swap(lhs.values_, rhs.values_);
            }

            //=== access ===//
            /// \returns An array view to the keys.
            sorted_view<const Key, Compare> keys() const noexcept
            {
                return keys_;
            }

            /// \returns An array view to the mapped values.
            /// \group values
            array_view<Value> values() noexcept
            {
                return values_;
            }
            /// \group values
            array_view<const Value> values() const noexcept
            {
                return values_;
            }

            iterator begin() noexcept
            {
                auto key_ptr   = iterator_to_pointer(keys_.begin());
                auto value_ptr = iterator_to_pointer(values_.begin());
                return iterator(typename iterator::private_key{}, key_ptr, value_ptr);
            }
            const_iterator begin() const noexcept
            {
                return cbegin();
            }
            const_iterator cbegin() const noexcept
            {
                auto key_ptr   = iterator_to_pointer(keys_.begin());
                auto value_ptr = iterator_to_pointer(values_.begin());
                return const_iterator(typename const_iterator::private_key{}, key_ptr, value_ptr);
            }

            iterator end() noexcept
            {
                auto key_ptr   = iterator_to_pointer(keys_.end());
                auto value_ptr = iterator_to_pointer(values_.end());
                return iterator(typename iterator::private_key{}, key_ptr, value_ptr);
            }
            const_iterator end() const noexcept
            {
                return cend();
            }
            const_iterator cend() const noexcept
            {
                auto key_ptr   = iterator_to_pointer(keys_.end());
                auto value_ptr = iterator_to_pointer(values_.end());
                return const_iterator(typename const_iterator::private_key{}, key_ptr, value_ptr);
            }

            /// \returns The key iterator corresponding to the given iterator.
            /// \requires The iterator must be an iterator in this map.
            /// \group key_iter
            key_const_iterator key_iter(const_iterator iter) const noexcept
            {
                return keys_.begin() + index_of(iter);
            }

            /// \returns The key iterator corresponding to the given iterator.
            /// \requires The iterator must be an iterator in this map.
            /// \group value_iter
            value_iterator value_iter(const_iterator iter) noexcept
            {
                return values_.begin() + index_of(iter);
            }
            /// \group value_iter
            value_const_iterator value_iter(const_iterator iter) const noexcept
            {
                return values_.begin() + index_of(iter);
            }

            key_iterator key_begin() noexcept
            {
                return keys_.begin();
            }
            key_const_iterator key_begin() const noexcept
            {
                return keys_.begin();
            }
            key_const_iterator key_cbegin() const noexcept
            {
                return keys_.cbegin();
            }

            key_iterator key_end() noexcept
            {
                return keys_.end();
            }
            key_const_iterator key_end() const noexcept
            {
                return keys_.end();
            }
            key_const_iterator key_cend() const noexcept
            {
                return keys_.cend();
            }

            /// \group value_iter
            value_iterator value_iter(key_const_iterator iter) noexcept
            {
                return values_.begin() + index_of(iter);
            }
            /// \group value_iter
            value_const_iterator value_iter(key_const_iterator iter) const noexcept
            {
                return values_.begin() + index_of(iter);
            }

            /// \returns The iterator corresponding to the given key or value iterator.
            /// \requires The iterator must be an iterator in this map.
            /// \group key_value_iter
            iterator key_value_iter(key_const_iterator iter) noexcept
            {
                return iterator(typename iterator::private_key{}, iterator_to_pointer(iter),
                                iterator_to_pointer(value_iter(iter)));
            }
            /// \group key_value_iter
            const_iterator key_value_iter(key_const_iterator iter) const noexcept
            {
                return const_iterator(typename const_iterator::private_key{},
                                      iterator_to_pointer(iter),
                                      iterator_to_pointer(value_iter(iter)));
            }

            value_iterator value_begin() noexcept
            {
                return values_.begin();
            }
            value_const_iterator value_begin() const noexcept
            {
                return values_.begin();
            }
            value_const_iterator value_cbegin() const noexcept
            {
                return values_.cbegin();
            }

            value_iterator value_end() noexcept
            {
                return values_.end();
            }
            value_const_iterator value_end() const noexcept
            {
                return values_.end();
            }
            value_const_iterator value_cend() const noexcept
            {
                return values_.cend();
            }

            /// \group key_iter
            key_const_iterator key_iter(value_const_iterator iter) const noexcept
            {
                return keys_.begin() + index_of(iter);
            }

            /// \group key_value_iter
            iterator key_value_iter(value_const_iterator iter) noexcept
            {
                return iterator(typename iterator::private_key{},
                                iterator_to_pointer(key_iter(iter)),
                                const_cast<Value*>(iterator_to_pointer(iter)));
            }
            /// \group key_value_iter
            const_iterator key_value_iter(value_const_iterator iter) const noexcept
            {
                return const_iterator(typename const_iterator::private_key{},
                                      iterator_to_pointer(key_iter(iter)),
                                      iterator_to_pointer(iter));
            }

            /// \returns The key value pair with the minimal key.
            /// \group min
            key_value_ref<Key, Value> min() noexcept
            {
                return {keys_.min(), values_.front()};
            }
            /// \group min
            key_value_ref<Key, const Value> min() const noexcept
            {
                return {keys_.min(), values_.front()};
            }

            /// \returns The key value pair with the maximal key.
            /// \group max
            key_value_ref<Key, Value> max() noexcept
            {
                return {keys_.max(), values_.back()};
            }
            /// \group max
            key_value_ref<Key, const Value> max() const noexcept
            {
                return {keys_.max(), values_.back()};
            }

            //=== capacity ===//
            /// \returns Whether or not the map is empty.
            bool empty() const noexcept
            {
                return keys_.empty();
            }

            /// \returns The number of elements in the map.
            size_type size() const noexcept
            {
                return keys_.size();
            }

            /// \returns The number of elements the map can contain without reserving new memory.
            size_type capacity() const noexcept
            {
                return std::min(keys_.capacity(), values_.capacity());
            }

            /// \returns The maximum number of elements as determined by the block storage.
            size_type max_size() const noexcept
            {
                return std::min(keys_.max_size(), values_.max_size());
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity` if that isn't the case already.
            void reserve(size_type new_capacity)
            {
                keys_.reserve(new_capacity);
                values_.reserve(new_capacity);
            }

            /// \effects Non-binding request to make the capacity as small as necessary.
            void shrink_to_fit()
            {
                keys_.shrink_to_fit();
                values_.shrink_to_fit();
            }

            //=== modifiers ===//
            /// The result of an insert operation.
            class insert_result
            {
            public:
                /// \returns An iterator to the key-value-pair with the given key.
                iterator iter() const noexcept
                {
                    return iter_;
                }

                /// \returns An iterator the value belonging to the given key.
                value_iterator value_iter() const noexcept
                {
                    return pointer_to_iterator<value_iterator>(
                        iter_.get_value_pointer(typename iterator::private_key{}));
                }

                /// \returns Whether or not the key was already present in the map.
                bool was_duplicate() const noexcept
                {
                    return was_duplicate_;
                }

                /// \returns Whether or not the key was inserted into the map.
                /// If `was_duplicate() == false`, this is `true`.
                /// Otherwise it is only true if the set allows duplicates.
                bool was_inserted() const noexcept
                {
                    return !was_duplicate_ || AllowDuplicates;
                }

            private:
                insert_result(iterator iter, bool dup) : iter_(iter), was_duplicate_(dup) {}

                iterator iter_;
                bool     was_duplicate_;

                friend flat_map;
            };

            /// \effects Does a lookup for the given key.
            /// If the key isn't part of the map or the map allows duplicates, inserts the key-value-pair
            /// where the key is constructed from the transparent key and the value is constructed from the arguments.
            /// Otherwise, does nothing.
            /// \returns The result of the insert operation.
            template <typename TransparentKey, typename... ValueArgs>
            insert_result try_emplace(TransparentKey&& key, ValueArgs&&... args)
            {
                auto result = keys_.try_emplace(std::forward<TransparentKey>(key));
                if (result.was_inserted())
                {
                    // key was inserted, insert value as well
                    auto iter = value_iter(result.iter());
                    values_.emplace(iter, std::forward<ValueArgs>(args)...);
                    return insert_result(key_value_iter(result.iter()), false);
                }
                else
                    // key was not inserted, don't insert value
                    return insert_result(key_value_iter(result.iter()), true);
            }

            /// \effects Does a lookup for the given key.
            /// If the key isn't part of the map or the map allows duplicates, inserts the key-value-pair
            /// where the key is constructed from the transparent key and the value is constructed from the arguments.
            /// Otherwise, assigns the value already stored to the value constructed by from the arguments.
            /// \returns The result of the insert operation.
            /// \requires The map must not allow duplicates.
            template <typename TransparentKey, typename... Args>
            insert_result emplace_or_assign(TransparentKey&& key, Args&&... args)
            {
                static_assert(sizeof(key) == sizeof(key) && !AllowDuplicates,
                              "emplace_or_assign doesn't make sense on multi maps");
                auto result =
                    try_emplace(std::forward<TransparentKey>(key), std::forward<Args>(args)...);
                if (!result.was_inserted())
                {
                    // update the value
                    // the forwarded arguments are not actually used, so fine to forward them again
                    assign_value(result.value_iter(), std::forward<Args>(args)...);
                }
                return result;
            }

            /// \effects Same as `try_emplace(FWD(k), FWD(v))`.
            template <
                typename K, typename V,
                typename = typename std::enable_if<std::is_convertible<K, Key>::value
                                                   && std::is_convertible<V, Value>::value>::type>
            insert_result insert(K&& k, V&& v)
            {
                return try_emplace(std::forward<K>(k), std::forward<V>(v));
            }

            /// \effects Same as `emplace_or_assign(FWD(k), FWD(v))`.
            template <
                typename K, typename V,
                typename = typename std::enable_if<std::is_convertible<K, Key>::value
                                                   && std::is_convertible<V, Value>::value>::type>
            insert_result insert_or_assign(K&& k, V&& v)
            {
                return emplace_or_assign(std::forward<K>(k), std::forward<V>(v));
            }

            /// \effects Same as `try_emplace(get<0>(FWD(pair)), get<1>(FWD(pair))`.
            template <class Pair>
            insert_result insert_pair(Pair&& pair)
            {
                using pair_type = typename std::decay<Pair>::type;
                static_assert(std::tuple_size<pair_type>::value == 2u, "not a pair");

                using std::get;
                return try_emplace(get<0>(std::forward<Pair>(pair)),
                                   get<1>(std::forward<Pair>(pair)));
            }

            /// \effects Same as `emplace_or_assign(get<0>(FWD(pair)), get<1>(FWD(pair))`.
            template <class Pair>
            insert_result insert_or_assign_pair(Pair&& pair)
            {
                using pair_type = typename std::decay<Pair>::type;
                static_assert(std::tuple_size<pair_type>::value == 2u, "not a pair");

                using std::get;
                return emplace_or_assign(get<0>(std::forward<Pair>(pair)),
                                         get<1>(std::forward<Pair>(pair)));
            }

            /// \effects Inserts keys from the range `[key_begin, key_end)` combined with the matching values from `[value_begin, value_end)`.
            /// It will stop as soon as one range is exhausted.
            template <typename KeyInputIt, typename ValueInputIt>
            void insert_range(KeyInputIt key_begin, KeyInputIt key_end, ValueInputIt value_begin,
                              ValueInputIt value_end)
            {
                auto no_keys =
                    range_size(typename std::iterator_traits<KeyInputIt>::iterator_category{},
                               key_begin, key_end);
                auto no_values =
                    range_size(typename std::iterator_traits<KeyInputIt>::iterator_category{},
                               value_begin, value_end);

                auto min = std::min(no_keys, no_values);
                reserve(size() + min);

                while (key_begin != key_end && value_begin != value_end)
                {
                    insert(*key_begin, *value_begin);
                    ++key_begin;
                    ++value_begin;
                }
            }

            /// \effects Inserts all elements in the range `[begin, end)` by calling `insert_pair(*cur)`.
            template <typename InputIt>
            void insert_pair_range(InputIt begin, InputIt end)
            {
                reserve(size()
                        + range_size(typename std::iterator_traits<InputIt>::iterator_category{},
                                     begin, end));
                for (auto cur = begin; cur != end; ++cur)
                    insert_pair(*cur);
            }

            /// \effects Destroys and removes all elements.
            void clear() noexcept
            {
                keys_.clear();
                values_.clear();
            }

            /// \effects Destroys and removes the element at the given position.
            /// \returns An iterator after the element that was removed.
            iterator erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable<Key>::value)
            {
                auto key_after = keys_.erase(key_iter(pos));
                values_.erase(value_iter(pos));
                return key_value_iter(key_after);
            }

            /// \effects Destroys and removes all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            iterator erase_range(const_iterator begin, const_iterator end) noexcept(
                std::is_nothrow_move_assignable<Key>::value)
            {
                auto value_begin = value_iter(begin);
                auto value_end   = value_iter(end);
                auto key_after   = keys_.erase_range(key_iter(begin), key_iter(end));
                values_.erase_range(value_begin, value_end);
                return key_value_iter(key_after);
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

            /// \effects Conceptually the same as `flat_map<Key, Value> m; m.insert_range(key_begin, key_end, value_begin, value_end); *this = std::move(m);`
            template <typename KeyInputIt, typename ValueInputIt>
            void assign_range(KeyInputIt key_begin, KeyInputIt key_end, ValueInputIt value_begin,
                              ValueInputIt value_end)
            {
                // TODO: exception safety
                clear();
                insert_range(key_begin, key_end, value_begin, value_end);
            }

            /// \effects Conceptually the same as `flat_map<Key, Value> m; m.insert_pair_range(begin, end); *this = std::move(m);`
            template <typename InputIt>
            void assign_pair_range(InputIt begin, InputIt end)
            {
                // TODO: exception safety
                clear();
                insert_pair_range(begin, end);
            }

            //=== lookup ===//
            /// \returns Whether or not the key is contained in the map.
            template <typename TransparentKey>
            bool contains(const TransparentKey& key) const noexcept
            {
                return find(key) != end();
            }

            /// \returns The value belonging to the given key.
            /// \requires The key must be stored in the map.
            /// \group lookup
            template <typename TransparentKey>
            Value& lookup(const TransparentKey& key) noexcept
            {
                auto iter = find(key);
                assert(iter != end());
                return iter->value;
            }

            /// \group lookup
            template <typename TransparentKey>
            const Value& lookup(const TransparentKey& key) const noexcept
            {
                auto iter = find(key);
                assert(iter != end());
                return iter->value;
            }

            /// \returns A pointer to the value belonging to the given key, or `nullptr`, if there was none.
            /// \group try_lookup
            template <typename TransparentKey>
            Value* try_lookup(const TransparentKey& key) noexcept
            {
                auto iter = find(key);
                if (iter == end())
                    return nullptr;
                else
                    return &iter->value;
            }

            /// \group try_lookup
            template <typename TransparentKey>
            const Value* try_lookup(const TransparentKey& key) const noexcept
            {
                auto iter = find(key);
                if (iter == end())
                    return nullptr;
                else
                    return &iter->value;
            }

            /// \returns An iterator to the given key-value-pair, or `end()` if the key is not in the map.
            /// \group find
            template <typename TransparentKey>
            iterator find(const TransparentKey& key) noexcept
            {
                return key_value_iter(keys_.find(key));
            }
            /// \group find
            template <typename TransparentKey>
            const_iterator find(const TransparentKey& key) const noexcept
            {
                return key_value_iter(keys_.find(key));
            }

            /// \returns The number of occurences of `key` in the map.
            /// \notes If `Compare::allow_duplicates == std::false_type` this is either `0` or `1`.
            template <typename TransparentKey>
            size_type count(const TransparentKey& key) const noexcept
            {
                return keys_.count(key);
            }

            /// \returns Same as [array::lower_bound]() for the given `key`.
            /// \group lower_bound
            template <typename TransparentKey>
            iterator lower_bound(const TransparentKey& key) noexcept
            {
                return key_value_iter(keys_.lower_bound(key));
            }
            /// \group lower_bound
            template <typename TransparentKey>
            const_iterator lower_bound(const TransparentKey& key) const noexcept
            {
                return key_value_iter(keys_.lower_bound(key));
            }

            /// \returns Same as [array::upper_bound]() for the given `key`.
            /// \group upper_bound
            template <typename TransparentKey>
            iterator upper_bound(const TransparentKey& key) noexcept
            {
                return key_value_iter(keys_.upper_bound(key));
            }
            /// \group upper_bound
            template <typename TransparentKey>
            const_iterator upper_bound(const TransparentKey& key) const noexcept
            {
                return key_value_iter(keys_.upper_bound(key));
            }

            /// \returns Same as [array::equal_range]() for the given `key`.
            /// \group equal_range
            template <typename TransparentKey>
            iter_pair<iterator> equal_range(const TransparentKey& key) noexcept
            {
                auto range = keys_.equal_range(key);
                return {key_value_iter(range.begin()), key_value_iter(range.end())};
            }
            /// \group equal_range
            template <typename TransparentKey>
            iter_pair<const_iterator> equal_range(const TransparentKey& key) const noexcept
            {
                auto range = keys_.equal_range(key);
                return {key_value_iter(range.begin()), key_value_iter(range.end())};
            }

        private:
            size_type index_of(key_const_iterator iter) const noexcept
            {
                assert(iter >= keys_.begin() && iter <= keys_.end());
                return size_type(iter - keys_.begin());
            }

            size_type index_of(value_const_iterator iter) const noexcept
            {
                assert(iter >= values_.begin() && iter <= values_.end());
                return size_type(iter - values_.begin());
            }

            size_type index_of(const_iterator iter) const noexcept
            {
                auto key = iter.get_key_pointer(typename const_iterator::private_key{});
                assert(key >= iterator_to_pointer(keys_.begin())
                       && key <= iterator_to_pointer(keys_.end()));
                return size_type(key - iterator_to_pointer(keys_.begin()));
            }

            template <typename Arg>
            auto assign_value(value_iterator iter, Arg&& arg) noexcept
                -> decltype(*iter = std::forward<Arg>(arg))
            {
                return *iter = std::forward<Arg>(arg);
            }
            template <typename... Args>
            void assign_value(value_iterator iter, Args&&... args) const
            {
                *iter = Value(std::forward<Args>(args)...);
            }

            template <typename InputIt>
            static size_type range_size(std::input_iterator_tag, InputIt, InputIt)
            {
                return size_type(0u);
            }

            template <typename ForwardIt>
            static size_type range_size(std::forward_iterator_tag, ForwardIt begin, ForwardIt end)
            {
                return size_type(std::distance(begin, end));
            }

            key_storage   keys_;
            value_storage values_;
        };

        /// Convenience typedef for an [array::flat_map]() that allows duplicates.
        template <typename Key, typename Value, typename Compare = key_compare_default,
                  class BlockStorage = block_storage_default>
        using flat_multimap = flat_map<Key, Value, Compare, BlockStorage, true>;
    } // namespace array
} // namespace foonathan

namespace std
{
    template <typename Key, typename Value>
    class tuple_size<foonathan::array::key_value_ref<Key, Value>>
    : public std::integral_constant<std::size_t, 2u>
    {
    };

    template <std::size_t I, typename Key, typename Value>
    class tuple_element<I, foonathan::array::key_value_ref<Key, Value>>
    {
    public:
        using type = typename foonathan::array::detail::get_key_value<
            I, foonathan::array::key_value_ref<Key, Value>>::type;
    };
} // namespace std

#endif // FOONATHAN_ARRAY_FLAT_MAP_HPP_INCLUDED
