// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_VARIANT_BAG_HPP_INCLUDED
#define FOONATHAN_ARRAY_VARIANT_BAG_HPP_INCLUDED

#include <foonathan/array/detail/all_of.hpp>
#include <foonathan/array/bag.hpp>

namespace foonathan
{
    namespace array
    {
        /// Tag type that just denotes a given type.
        ///
        /// It can be used to call the member functions of [array::variant_bag]()
        /// if you don't pass in the type as template argument.
        ///
        /// For example, you can write `bag.size<int>()` to get the number of ints,
        /// but also `bag.size(type<int>{})` or use a custom implementation like `bag.size(boost::hana::type<int>{})`.
        template <typename T>
        struct type_t
        {
            constexpr type_t() = default;
        };

#if FOONATHAN_ARRAY_HAS_VARIABLE_TEMPLATES
        /// Tag object that denotes a given type.
        template <typename T>
        constexpr auto type = type_t<T>{};
#endif

        /// Tag type that just denotes a list of types.
        template <typename... Types>
        struct type_list
        {
        };

        namespace detail
        {
            template <typename T, class BlockStorage>
            class variant_bag_single
            {
            public:
                variant_bag_single() = default;

                explicit variant_bag_single(argument_type<BlockStorage> arg) noexcept : bag(arg) {}

                friend void swap(variant_bag_single& lhs, variant_bag_single& rhs) noexcept(
                    block_storage_nothrow_move<BlockStorage, T>::value)
                {
                    swap(lhs.bag, rhs.bag);
                }

                foonathan::array::bag<T, BlockStorage> bag;
            };
        } // namespace detail

        /// A heterogeneous bag of elements.
        ///
        /// It is essentially a tuple of [array::bag](), one for each of the specified types.
        /// Inserts and erase are (amortized) O(1), but the order of elements is not determined.
        template <class BlockStorage, typename... Types>
        class variant_bag : detail::variant_bag_single<Types, BlockStorage>...
        {
            using nothrow_move =
                detail::all_of<block_storage_nothrow_move<BlockStorage, Types>::value...>;

#define FOONATHAN_ARRAY_FOR_EACH(...)                                                              \
    int dummy[] = {((__VA_ARGS__), 0)...};                                                         \
    (void)dummy

            template <typename T>
            struct is_stored
            : std::is_base_of<detail::variant_bag_single<T, BlockStorage>, variant_bag>
            {
            };

            class iterator_tag
            {
                constexpr iterator_tag() = default;

                friend variant_bag;
            };

        public:
            using value_types   = type_list<Types...>;
            using block_storage = BlockStorage;

            template <typename T>
            using iterator = pointer_iterator<iterator_tag, T>;
            template <typename T>
            using const_iterator = pointer_iterator<iterator_tag, const T>;

            //=== constructors/destructors ===//
            /// Default constructor.
            /// \effects Creates a bag without any elements.
            /// The block storage is initialized with default constructed arguments.
            variant_bag() = default;

            /// \effects Creates a bag without any elements.
            /// The block storage is initialized with the given arguments.
            explicit variant_bag(argument_type<BlockStorage> arg) noexcept
            : detail::variant_bag_single<Types, BlockStorage>(arg)...
            {
            }

            /// Swap.
            friend void swap(variant_bag& lhs, variant_bag& rhs) noexcept(nothrow_move::value)
            {
                FOONATHAN_ARRAY_FOR_EACH(
                    swap(lhs.get_single(type_t<Types>{}), rhs.get_single(type_t<Types>{})));
            }

            //=== access ===//
            /// \returns A block view to the elements of the given type.
            template <template <typename> class Tag, typename T>
            block_view<T> operator()(Tag<T>) noexcept
            {
                return get_single(type_t<T>{});
            }
            /// \returns A `const` block view to the elements of the given type.
            template <template <typename> class Tag, typename T>
            block_view<const T> operator()(Tag<T>) const noexcept
            {
                return get_single(type_t<T>{});
            }

            /// \returns A block view to the elements of the given type.
            template <typename T, template <typename> class Tag = type_t>
            block_view<T> view(Tag<T> = type_t<T>{}) noexcept
            {
                return get_single(type_t<T>{});
            }
            /// \returns A `const` block view to the elements of the given type.
            template <typename T, template <typename> class Tag = type_t>
            block_view<const T> view(Tag<T> = type_t<T>{}) const noexcept
            {
                return get_single(type_t<T>{});
            }

            template <typename T, template <typename> class Tag = type_t>
            iterator<T> begin(Tag<T> tag = type_t<T>{})
            {
                auto ptr = iterator_to_pointer(view(tag).begin());
                return pointer_to_iterator<iterator<T>>(ptr);
            }
            template <typename T, template <typename> class Tag = type_t>
            const_iterator<T> begin(Tag<T> tag = type_t<T>{}) const
            {
                auto ptr = iterator_to_pointer(view(tag).begin());
                return pointer_to_iterator<const_iterator<T>>(ptr);
            }
            template <typename T, template <typename> class Tag = type_t>
            const_iterator<T> cbegin(Tag<T> tag = type_t<T>{}) const
            {
                auto ptr = iterator_to_pointer(view(tag).begin());
                return pointer_to_iterator<const_iterator<T>>(ptr);
            }

            template <typename T, template <typename> class Tag = type_t>
            iterator<T> end(Tag<T> tag = type_t<T>{})
            {
                auto ptr = iterator_to_pointer(view(tag).end());
                return pointer_to_iterator<iterator<T>>(ptr);
            }
            template <typename T, template <typename> class Tag = type_t>
            const_iterator<T> end(Tag<T> tag = type_t<T>{}) const
            {
                auto ptr = iterator_to_pointer(view(tag).end());
                return pointer_to_iterator<const_iterator<T>>(ptr);
            }
            template <typename T, template <typename> class Tag = type_t>
            const_iterator<T> cend(Tag<T> tag = type_t<T>{}) const
            {
                auto ptr = iterator_to_pointer(view(tag).end());
                return pointer_to_iterator<const_iterator<T>>(ptr);
            }

            //=== capacity ===//
            /// \returns Whether or not the bag is completely empty.
            bool empty() const noexcept
            {
                bool result = true;
                FOONATHAN_ARRAY_FOR_EACH(result &= get_single(type_t<Types>{}).empty());
                return result;
            }

            /// \returns Whether or not the bag has no elements of this specific type.
            template <typename T, template <typename> class Tag = type_t>
            bool empty(Tag<T> = type_t<T>{}) const noexcept
            {
                return get_single(type_t<T>{}).empty();
            }

            /// \returns The number of elements in the bag, all types.
            size_type size() const noexcept
            {
                auto result = size_type(0);
                FOONATHAN_ARRAY_FOR_EACH(result += get_single(type_t<Types>{}).size());
                return result;
            }

            /// \returns The number of elements in the bag, this type.
            template <typename T, template <typename> class Tag = type_t>
            size_type size(Tag<T> = type_t<T>{}) const noexcept
            {
                return get_single(type_t<T>{}).size();
            }

            /// \returns The number of elements the bag can contain of this specific type without reserving new memory.
            template <typename T, template <typename> class Tag = type_t>
            size_type capacity(Tag<T> = type_t<T>{}) const noexcept
            {
                return get_single(type_t<T>{}).capacity();
            }

            /// \returns The maximum number of elements of this type as determined by the block storage.
            template <typename T, template <typename> class Tag = type_t>
            size_type max_size(Tag<T> = type_t<T>{}) const noexcept
            {
                return get_single(type_t<T>{}).max_size();
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity`, all types.
            void reserve(size_type new_capacity)
            {
                FOONATHAN_ARRAY_FOR_EACH(get_single(type_t<Types>{}).reserve(new_capacity));
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity`, this type.
            /// \group reserve
            template <typename T, template <typename> class Tag = type_t>
            void reserve(Tag<T>, size_type new_capacity)
            {
                get_single(type_t<T>{}).reserve(new_capacity);
            }
            /// \group reserve
            template <typename T>
            void reserve(size_type new_capacity)
            {
                get_single(type_t<T>{}).reserve(new_capacity);
            }

            /// \effects Non-binding request to make the capacity as small as necessary, all types.
            void shrink_to_fit()
            {
                FOONATHAN_ARRAY_FOR_EACH(get_single(type_t<Types>{}).shrink_to_fit());
            }

            /// \effects Non-binding request to make the capacity as small as necessary, this type.
            template <typename T, template <typename> class Tag = type_t>
            void shrink_to_fit(Tag<T> = type_t<T>{})
            {
                get_single(type_t<T>{}).shrink_to_fit();
            }

            //=== modifiers ===//
            /// \effects Creates a new element of the specified type in the proper bag.
            /// \group emplace
            template <template <typename> class Tag, typename T, typename... Args>
            T& emplace(Tag<T>, Args&&... args)
            {
                static_assert(std::is_empty<Tag<T>>::value,
                              "did you meant to call emplace<T>(args...)?");
                return get_single(type_t<T>{}).emplace(std::forward<Args>(args)...);
            }
            /// \group emplace
            template <typename T, typename... Args>
            T& emplace(Args&&... args)
            {
                return get_single(type_t<T>{}).emplace(std::forward<Args>(args)...);
            }

            /// \effects Same as `emplace<TYPE>(std::forward<T>(element))`, where `TYPE` is the decayed type of `T`.
            /// \notes It cannot be used to insert an object that is only convertible to one of the types,
            /// and not the type itself.
            template <typename T>
            auto insert(T&& element) ->
                typename std::enable_if<is_stored<typename std::decay<T>::type>::value>::type
            {
                using type = typename std::decay<T>::type;
                get_single(type_t<type>{}).insert(std::forward<T>(element));
            }

            /// \effects Same as `insert_range<T>(view.begin(), view.end())`.
            /// \group insert
            template <typename T>
            iterator<T> insert(const block_view<const T>& view)
            {
                return insert_range<T>(view.begin(), view.end());
            }
            /// \group insert
            template <typename T>
            iterator<T> insert(const block_view<T>& view)
            {
                return insert_range<T>(view.begin(), view.end());
            }

            /// \effects Inserts the elements in the sequence `[begin, end)` by calling `insert()` multiple times.
            /// \notes It cannot be used to insert an object that is only convertible to one of the types,
            /// and not the type itself.
            template <typename InputIt>
            void insert_range(InputIt begin, InputIt end)
            {
                for (auto cur = begin; cur != end; ++cur)
                    insert(*cur);
            }

            /// \effects Inserts the elements in the sequence `[begin, end)` by calling `emplace<T>()` repeatedly.
            /// \returns An iterator one past the last inserted element.
            /// \group insert_range
            template <template <typename> class Tag, typename T, typename InputIt>
            iterator<T> insert_range(Tag<T>, InputIt begin, InputIt end)
            {
                return get_single(type_t<T>{}).insert_range(begin, end);
            }
            /// \group insert_range
            template <typename T, typename InputIt>
            iterator<T> insert_range(InputIt begin, InputIt end)
            {
                return iterator<T>(iterator_tag{},
                                   get_single(type_t<T>{}).insert_range(begin, end));
            }

            /// \effects Destroys all elements of any type.
            void clear() noexcept
            {
                FOONATHAN_ARRAY_FOR_EACH(get_single(type_t<Types>{}).clear());
            }

            /// \effects Destroys all elements of this type.
            template <typename T, template <typename> class Tag = type_t>
            void clear(Tag<T> = type_t<T>{})
            {
                get_single(type_t<T>{}).clear();
            }

            /// \effects Destroys and removes the element at the given position.
            /// \returns An iterator after the element that was removed.
            /// \group erase
            template <typename T>
            iterator<T> erase(const_iterator<T> iter) noexcept(
                detail::is_nothrow_swappable<T>::value)
            {
                auto bag_iter =
                    iterator_cast<typename foonathan::array::bag<T, BlockStorage>::const_iterator>(
                        iter);
                return iterator<T>(iterator_tag{}, get_single(type_t<T>{}).erase(bag_iter));
            }
            /// \group erase
            template <typename T>
            iterator<T> erase(iterator<T> iter) noexcept(detail::is_nothrow_swappable<T>::value)
            {
                return erase(const_iterator<T>(iter));
            }

            /// \effects Destroys all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            /// \group erase_range
            template <typename T>
            iterator<T> erase_range(const_iterator<T> begin, const_iterator<T> end) noexcept(
                std::is_nothrow_move_assignable<T>::value)
            {
                auto bag_begin =
                    iterator_cast<typename foonathan::array::bag<T, BlockStorage>::const_iterator>(
                        begin);
                auto bag_end =
                    iterator_cast<typename foonathan::array::bag<T, BlockStorage>::const_iterator>(
                        end);
                return iterator<T>(iterator_tag{},
                                   get_single(type_t<T>{}).erase_range(bag_begin, bag_end));
            }
            /// \group erase_range
            template <typename T>
            iterator<T> erase_range(iterator<T> begin, iterator<T> end) noexcept(
                std::is_nothrow_move_assignable<T>::value)
            {
                return erase_range(const_iterator<T>(begin), const_iterator<T>(end));
            }

        private:
            template <typename T>
            foonathan::array::bag<T, BlockStorage>& get_single(type_t<T>) noexcept
            {
                static_assert(is_stored<T>::value, "type is not stored in variant_bag");
                return static_cast<detail::variant_bag_single<T, BlockStorage>&>(*this).bag;
            }
            template <typename T>
            const foonathan::array::bag<T, BlockStorage>& get_single(type_t<T>) const noexcept
            {
                static_assert(is_stored<T>::value, "type is not stored in variant_bag");
                return static_cast<const detail::variant_bag_single<T, BlockStorage>&>(*this).bag;
            }

#undef FOONATHAN_ARRAY_FOR_EACH
        };

        namespace detail
        {
            template <class List, class BlockStorage>
            struct variant_bag_tl;

            template <template <typename...> class Templ, typename... Args, class BlockStorage>
            struct variant_bag_tl<Templ<Args...>, BlockStorage>
            {
                using type = variant_bag<BlockStorage, Args...>;
            };
        } // namespace detail

        /// Alias to specify an [array::variant_bag]() from a type list.
        ///
        /// The first argument can be any type with any number of template type arguments,
        /// it will take the arguments and pass them as the types for the bag.
        ///
        /// So, for example, `variant_bag_tl<std::tuple<int, float, char>>`,
        /// or `variant_bag<type_list<int, float, char>>`.
        template <class List, class BlockStorage = block_storage_default>
        using variant_bag_tl = typename detail::variant_bag_tl<List, BlockStorage>::type;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_VARIANT_BAG_HPP_INCLUDED
