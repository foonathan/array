// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <tuple>

#include <foonathan/array/block_view.hpp>
#include <foonathan/array/raw_storage.hpp>

namespace foonathan
{
    namespace array
    {
        //=== block_storage_args ===//
        /// Tag type to store a collection of arguments to create a `BlockStorage`.
        /// \notes These can be things like runtime parameters or references to allocators.
        template <typename... Args>
        struct block_storage_args_t
        {
            std::tuple<Args...> args;

            /// \effects Creates the default set of arguments,
            /// it may be ill-formed.
            block_storage_args_t() = default;

            /// \effects Creates the arguments from a tuple.
            explicit block_storage_args_t(std::tuple<Args...> args) : args(std::move(args)) {}

            ~block_storage_args_t() noexcept = default;

            /// \effects Copies the arguments, it must not throw.
            block_storage_args_t(const block_storage_args_t&) noexcept = default;
            block_storage_args_t& operator=(const block_storage_args_t&) = default;
        };

        /// \returns The block storage arguments created by forwarding the given arguments to the tuple.
        template <typename... Args>
        block_storage_args_t<typename std::decay<Args>::type...> block_storage_args(
            Args&&... args) noexcept
        {
            return block_storage_args_t<typename std::decay<Args>::type...>(
                std::make_tuple(std::forward<Args>(args)...));
        }

        /// \returns The block storage arguments created by forwarding the given argument to the tuple.
        template <typename Arg>
        block_storage_args_t<typename std::decay<Arg>::type> block_storage_arg(Arg&& arg) noexcept
        {
            return block_storage_args_t<typename std::decay<Arg>::type>(
                std::make_tuple(std::forward<Arg>(arg)));
        }

        //=== block_storage_args_storage ===//
        namespace detail
        {
            template <bool... Bools>
            struct bool_list
            {
            };

            template <bool... Bools>
            using all_true = std::is_same<bool_list<true, Bools...>, bool_list<Bools..., true>>;

            template <typename... Args>
            using all_empty = all_true<std::is_empty<Args>::value...>;

            template <bool B, typename... Args>
            class arg_storage_impl
            {
                static_assert(sizeof...(Args) > 0u, "no arguments should mean that all are empty");

            public:
                using arg_type = block_storage_args_t<Args...>;

                explicit arg_storage_impl(arg_type arguments) noexcept
                : arguments_(std::move(arguments))
                {
                }

                void set_stored_arguments(arg_type arguments) noexcept
                {
                    arguments_ = std::move(arguments);
                }

                const arg_type& stored_arguments() const noexcept
                {
                    return arguments_;
                }
                arg_type& stored_arguments() noexcept
                {
                    return arguments_;
                }

            private:
                arg_type arguments_;
            };

            template <typename... Args>
            class arg_storage_impl<true, Args...>
            {
            public:
                using arg_type = block_storage_args_t<Args...>;

                explicit arg_storage_impl(const arg_type&) noexcept {}

                void set_stored_arguments(const arg_type&) noexcept {}

                arg_type stored_arguments() const noexcept
                {
                    return {};
                }
            };

            template <class Args>
            struct arg_storage;

            template <typename... Args>
            struct arg_storage<block_storage_args_t<Args...>>
            {
                using type = arg_storage_impl<all_empty<Args...>::value, Args...>;
            };
        } // namespace detail

        /// Optimized storage for [array::block_storage_args_t]().
        ///
        /// It is intended as a base class for EBO, only stores arguments if not all argument types are empty.
        template <class Args>
        using block_storage_args_storage = typename detail::arg_storage<Args>::type;
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
