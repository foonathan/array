// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_INPUT_VIEW_HPP_INCLUDED
#define FOONATHAN_ARRAY_INPUT_VIEW_HPP_INCLUDED

#include <foonathan/array/block_storage.hpp>

namespace foonathan
{
    namespace array
    {
        /// Tag type to mark an [array::block_view]() the [array::input_view]() will move from.
        struct move_tag
        {
        };

        namespace detail
        {
            template <class View, class Block, typename = void>
            struct can_steal_memory : std::false_type
            {
            };

            template <class View, class Block>
            struct can_steal_memory<View, Block, decltype(void(&Block::operator View))>
            : std::true_type
            {
            };
        } // namespace detail

        /// A view that allows stealing memory from another block storage.
        ///
        /// Use this instead of input parameters of your container type.
        /// When constructing the container from it, it will steal memory,
        /// if constructed from a block storage of the same type,
        /// move all elements individually if created from an [array::block_view]() marked as move,
        /// otherwise copy all elements individually.
        /// \notes Containers should write an rvalue qualified conversion operator to the corresponding `input_view` to enable it.
        /// \notes It is designed to be used in function parameters only.
        template <typename T, class BlockStorage>
        class input_view
        {
            static_assert(!std::is_const<T>::value, "T must not be const qualified");

            template <class Block>
            using can_steal =
                std::integral_constant<bool,
                                       !std::is_const<Block>::value
                                           && detail::can_steal_memory<input_view, Block>::value>;

            template <class Block>
            using use_block_view = std::integral_constant<
                bool, !can_steal<Block>::value
                          && std::is_same<const T, const block_value_type<Block>>::value>;

        public:
            using value_type    = T;
            using block_storage = BlockStorage;

            /// \effects Creates it giving it a block storage it will steal from.
            /// It assumes ownership over the storage and the constructed elements.
            /// \requires The block storage must live as least as long as the view.
            /// \notes Use this constructor to implement the implicit conversion to `input_view`.
            input_view(BlockStorage&& storage, block_view<T> constructed) noexcept
            : storage_ptr_(&storage), constructed_(constructed)
            {
            }

            /// \effects Creates it giving it a view it will use as input.
            /// It will move the elements from that view.
            input_view(move_tag, block_view<T> input) noexcept
            : storage_ptr_(move_marker()), constructed_(input)
            {
            }

            /// \effects Creates it giving it a view it will use as input.
            /// It will copy the elements from that view.
            input_view(block_view<const T> input) noexcept
            // const_cast is okay, it will never try to modify the elements
            : storage_ptr_(nullptr), constructed_(const_cast<T*>(input.data()), input.size())
            {
            }

            /// \effects Creates it from the block, it will copy the elements.
            template <typename Block,
                      typename = typename std::enable_if<use_block_view<const Block>::value>::type>
            input_view(const Block& input) noexcept : input_view(block_view<const T>(input))
            {
            }

            /// \effects Creates it from the block stealing its elements.
            template <typename Block,
                      typename = typename std::enable_if<!std::is_reference<Block>::value
                                                         && can_steal<Block>::value>::type>
            input_view(Block&& input) noexcept : input_view(std::move(input).operator input_view())
            {
            }

            /// \effects Creates it giving it a [std::initializer_list]() it will use as input.
            /// It will copy the elements from that list.
            input_view(std::initializer_list<T> input) noexcept
            : input_view(block_view<const T>(input))
            {
            }

            input_view(input_view&& other) noexcept
            : storage_ptr_(other.storage_ptr_), constructed_(other.constructed_)
            {
                other.storage_ptr_ = nullptr;
                other.constructed_ = block_view<T>();
            }

            input_view& operator=(input_view&& other) noexcept
            {
                storage_ptr_       = other.storage_ptr_;
                constructed_       = other.constructed_;
                other.storage_ptr_ = nullptr;
                other.constructed_ = block_view<T>();
            }

            ~input_view() noexcept
            {
                if (will_steal_memory())
                    clear_and_shrink(origin_storage(), constructed_);
            }

            /// \returns Whether or not the call to `release()` can steal memory from another storage.
            bool will_steal_memory() const noexcept
            {
                return storage_ptr_ != nullptr && storage_ptr_ != move_marker();
            }

            /// \returns Whether or not the call to `release()` can move objects.
            /// \notes If `will_steal_memory() == true`, this will always return `false`.
            bool will_move() const noexcept
            {
                return storage_ptr_ == move_marker();
            }

            /// \returns Whether or not the call to `release()` has to copy objects.
            /// \notes If `will_steal_memory() == true` or `will_move() == true`,
            /// this will always return `false`.
            bool will_copy() const noexcept
            {
                return storage_ptr_ == nullptr;
            }

            /// \returns The storage it can steal the memory from.
            /// \requires `can_steal_memory()`
            BlockStorage& origin_storage() const noexcept
            {
                return *static_cast<BlockStorage*>(storage_ptr_);
            }

            /// \effects If `can_steal_memory() == true`, transfers ownership of the memory and constructed objects to `dest`.
            /// Otherwise, allocates new memory and copy/moves the objects over.
            /// \returns A view on the now constructed objects in `dest`.
            /// This either views the same location as before (if `can_steal_memory() == true` and no embedded storage was used),
            /// or starts at the beginning of the memory of `dest`.
            /// \notes In particular, the view could not start at the beginning if it didn't start at the beginning in `dest`.
            /// \throws Anything thrown by the allocation function or `T`s copy/move constructor.
            block_view<T> release(BlockStorage& dest, block_view<T> dest_constructed) &&
            {
                if (will_steal_memory())
                {
                    // steal memory from storage
                    BlockStorage::swap(dest, dest_constructed, origin_storage(), constructed_);
                    // destroy the elements previously owned by the storage
                    destroy_range(constructed_.begin(), constructed_.end());

                    storage_ptr_ = nullptr;
                    constructed_ = block_view<T>();

                    return dest_constructed;
                }
                else if (will_move())
                    return assign_move_impl(0, dest, dest_constructed);
                else
                    return assign_copy_impl(0, dest, dest_constructed);
            }

        private:
            static void* move_marker() noexcept
            {
                static char dummy;
                return &dummy;
            }

            template <typename U>
            auto assign_move_impl(int, BlockStorage& dest, block_view<U> dest_constructed) ->
                typename std::enable_if<std::is_move_constructible<U>::value, block_view<U>>::type
            {
                return assign_move(dest, dest_constructed, constructed_.begin(),
                                   constructed_.end());
            }
            template <typename U>
            block_view<U> assign_move_impl(short, BlockStorage&, block_view<U>)
            {
                assert(false && "trying to move non-moveable type");
                return {};
            }

            template <typename U>
            auto assign_copy_impl(int, BlockStorage& dest, block_view<U> dest_constructed) ->
                typename std::enable_if<std::is_copy_constructible<U>::value, block_view<U>>::type
            {
                return assign_copy(dest, dest_constructed, constructed_.begin(),
                                   constructed_.end());
            }
            template <typename U>
            block_view<U> assign_copy_impl(short, BlockStorage&, block_view<U>)
            {
                assert(false && "trying to copy non-copyable type");
                return {};
            }

            // if nullptr: copy elements
            // if move marker: move elements
            // otherwise: a valid storage
            void*         storage_ptr_;
            block_view<T> constructed_;
        };

        namespace detail
        {
            template <typename T>
            class move_t
            {
            public:
                constexpr move_t(move_tag, block_view<T> view) noexcept : view_(view) {}

                template <class BlockStorage>
                operator input_view<T, BlockStorage>() const noexcept
                {
                    return input_view<T, BlockStorage>(move_tag{}, view_);
                }

            private:
                block_view<T> view_;
            };
        } // namespace detail

        /// Similar to [std::move](), but for blocks.
        ///
        /// Use it to construct an [array::input_view]() that will move the given elements.
        /// \returns A proxy object convertible to `input_view<block_value_type<Block>, AnyBlockStorage>`.
        /// The input view will move the elements from the block, but does not steal the memory.
        /// \notes This function does not participate in overload resolution
        /// if `Block` is not convertible to an [array::block_view]().
        template <class Block>
        constexpr detail::move_t<block_value_type<Block>> move(Block& block) noexcept
        {
            static_assert(!std::is_const<Block>::value, "cannot move from const");
            return detail::move_t<block_value_type<Block>>(move_tag{}, block);
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_INPUT_VIEW_HPP_INCLUDED
