// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED

#include <tuple>

namespace foonathan
{
    namespace array
    {
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

            /// \notes Arguments don't need to be assignable.
            block_storage_args_t& operator=(const block_storage_args_t&) = delete;
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
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ARG_HPP_INCLUDED
