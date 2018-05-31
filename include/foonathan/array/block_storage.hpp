// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED

#include <type_traits>
#include <utility>

#include <foonathan/array/memory_block.hpp>

namespace foonathan
{
    namespace array
    {
        //=== BlockStorage traits ===//
        /// The default argument type of `BlockStorage.
        struct default_argument_type
        {
        };

        /// \exclude
        namespace traits_detail
        {
            template <class BlockStorage, typename = void>
            struct has_argument_type : std::false_type
            {
            };

            template <class BlockStorage>
            struct has_argument_type<
                BlockStorage, decltype(void(std::declval<typename BlockStorage::argument_type>()))>
            : std::true_type
            {
            };

            template <bool HasArgumentType, class BlockStorage>
            struct argument_type
            {
                using type = default_argument_type;
            };
            template <class BlockStorage>
            struct argument_type<true, BlockStorage>
            {
                using type = typename BlockStorage::argument_type;
            };

            template <class BlockStorage>
            auto argument_of(int, const BlockStorage& storage) ->
                typename BlockStorage::argument_type
            {
                return storage.argument();
            }
            template <class BlockStorage>
            default_argument_type argument_of(short, const BlockStorage&)
            {
                return {};
            }
        } // namespace traits_detail

        /// The argument type of a `BlockStorage`, or `default_argument_type` if it doesn't have one.
        /// \notes Use this instead of `BlockStorage::argument_type`, as it is optional.
        template <class BlockStorage>
        using argument_type = typename traits_detail::argument_type<
            traits_detail::has_argument_type<BlockStorage>::value, BlockStorage>::type;

        /// \returns The argument the `BlockStorage` was created with.
        /// \notes Use this instead of calling `argument()` of the block storage directly, as it is optional.
        template <class BlockStorage>
        argument_type<BlockStorage> argument_of(const BlockStorage& storage) noexcept
        {
            return traits_detail::argument_of(0, storage);
        }

        /// \exclude
        namespace traits_detail
        {
            struct low_prio
            {
            };
            struct mid_prio : low_prio
            {
            };
            struct high_prio : mid_prio
            {
            };

            template <class BlockStorage, class Arg>
            static auto max_size(high_prio, const Arg& arg) -> decltype(BlockStorage::max_size(arg))
            {
                return BlockStorage::max_size(arg);
            }
            template <class BlockStorage, class Arg>
            static auto max_size(mid_prio, const Arg&) -> decltype(BlockStorage::max_size())
            {
                return BlockStorage::max_size();
            }
            template <class BlockStorage, class Arg>
            static size_type max_size(low_prio, const Arg&)
            {
                return memory_block::max_size();
            }

        } // namespace traits_detail

        /// \returns The maximum size of a `BlockStorage` created with the given arguments.
        /// \notes Use this instead of calling `max_size()` of the block storage directly, as it is optional.
        template <class BlockStorage>
        size_type max_size(const argument_type<BlockStorage>& arg) noexcept
        {
            return traits_detail::max_size<BlockStorage>(traits_detail::high_prio{}, arg);
        }

        /// \returns The maximum size of the given `BlockStorage`.
        template <class BlockStorage>
        size_type max_size(const BlockStorage& storage) noexcept
        {
            return max_size<BlockStorage>(argument_of(storage));
        }

        //=== argument storage ===//
        namespace detail
        {
            template <typename Argument, typename = void>
            class argument_storage
            {
            public:
                using argument_type = Argument;

                explicit argument_storage(const Argument& arg) noexcept : argument_(arg) {}

                argument_storage(const argument_storage&) noexcept = default;
                argument_storage& operator=(const argument_storage&) noexcept = default;

                void set_stored_argument(argument_type argument) noexcept
                {
                    argument_ = std::move(argument);
                }

                const argument_type& stored_argument() const noexcept
                {
                    return argument_;
                }
                argument_type& stored_argument() noexcept
                {
                    return argument_;
                }

                void swap_argument(argument_storage& other) noexcept
                {
                    std::swap(argument_, other.argument_);
                }

            protected:
                ~argument_storage() = default;

            private:
                Argument argument_;
            };

            template <typename Argument>
            class argument_storage<Argument,
                                   typename std::enable_if<std::is_empty<Argument>::value>::type>
            {
                static_assert(std::is_default_constructible<Argument>::value,
                              "empty argument type must be default constructible");

            public:
                using argument_type = Argument;

                explicit argument_storage(const argument_type&) noexcept {}

                argument_storage(const argument_storage&) noexcept = default;
                argument_storage& operator=(const argument_storage&) noexcept = default;

                void set_stored_argument(argument_type) noexcept {}

                argument_type stored_argument() const noexcept
                {
                    return {};
                }

                void swap_argument(argument_storage&) noexcept {}

            protected:
                ~argument_storage() = default;
            };
        } // namespace detail

        /// Optimized storage for the argument of a block storage.
        ///
        /// It is intended to be a base class for EBO.
        template <class Argument>
        using argument_storage = detail::argument_storage<Argument>;

        /// Optimized storage for the arguments of a block storage.
        template <class BlockStorage>
        using argument_storage_for = argument_storage<argument_type<BlockStorage>>;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
