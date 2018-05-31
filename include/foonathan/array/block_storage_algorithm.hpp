// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED

#include <foonathan/array/block_view.hpp>
#include <foonathan/array/raw_storage.hpp>

namespace foonathan
{
    namespace array
    {
        /// `std::true_type` if move operations of a `BlockStorage` will never throw, `std::false_type` otherwise.
        ///
        /// They'll never throw if the storage does not use embedded storage or the type is nothrow move constructible.
        template <class BlockStorage, typename T>
        using block_storage_nothrow_move =
            std::integral_constant<bool, !embedded_storage<BlockStorage>::value
                                             || std::is_nothrow_move_constructible<T>::value>;

        /// \effects Clears a block storage by destroying all constructed objects and releasing the memory.
        template <class BlockStorage, typename T>
        void clear_and_shrink(BlockStorage& storage, block_view<T> constructed) noexcept
        {
            destroy_range(constructed.begin(), constructed.end());
            constructed = block_view<T>(storage.block().resize(0u));

            BlockStorage  empty(argument_of(storage));
            block_view<T> empty_constructed(empty.block().resize(0u));
            BlockStorage::swap(storage, constructed, empty, empty_constructed);
            // this will never throw as there are no objects that need moving

            // storage now owns no memory
            // empty now owns the memory of storage
            // destructor of empty will release memory of storage
        }

        /// \effects Destroys all created objects and increases the memory block so it has at least `new_size` elements.
        /// \returns A pointer to the new memory.
        template <class BlockStorage, typename T>
        raw_pointer clear_and_reserve(BlockStorage& storage, const block_view<T>& constructed,
                                      size_type new_size)
        {
            destroy_range(constructed.begin(), constructed.end());
            if (new_size <= storage.block().size())
                return storage.block().begin();
            else
            {
                storage.reserve(new_size - storage.block().size(), block_view<T>());
                return storage.block().begin();
            }
        }

        /// Normalizes a block by moving all constructed objects to the front.
        /// \effects Moves the elements currently constructed at `[constructed.begin(), constructed.end())`
        /// to `[storage.block().begin(), storage.block.begin() + constructed.size())`.
        /// \returns A view to the new location of the objects.
        /// \throws Anything thrown by the move constructor or assignment operator.
        template <class BlockStorage, typename T>
        block_view<T> move_to_front(BlockStorage& storage, block_view<T> constructed) noexcept(
            std::is_nothrow_move_constructible<T>::value)
        {
            auto offset = constructed.block().begin() - storage.block().begin();
            assert(offset >= 0);
            assert(std::size_t(offset) % sizeof(T) == 0);
            if (offset == 0)
                // already at the front
                return constructed;
            else if (offset >= std::ptrdiff_t(constructed.size() * sizeof(T)))
            {
                // doesn't overlap, just move forward
                auto new_end = uninitialized_destructive_move(constructed.begin(),
                                                              constructed.end(), storage.block());
                return block_view<T>(memory_block(storage.block().begin(), new_end));
            }
            else
            {
                // move construct the first offset elements at the correct location
                auto mid = constructed.begin() + offset / std::ptrdiff_t(sizeof(T));
                uninitialized_move(constructed.begin(), mid, storage.block());

                // now we can assign the next elements to the already moved ones
                auto new_end = std::move(mid, constructed.end(), constructed.begin());

                // destroy the unnecessary trailing elements
                destroy_range(new_end, constructed.end());

                return block_view<T>(to_pointer<T>(storage.block().begin()), constructed.size());
            }
        }

        namespace detail
        {
            template <class FwdIter, typename T>
            auto copy_or_move_assign(std::true_type, FwdIter begin, FwdIter end,
                                     const block_view<T>& dest) -> typename block_view<T>::iterator
            {
                return std::move(begin, end, dest.begin());
            }
            template <class FwdIter, typename T>
            auto copy_or_move_assign(std::false_type, FwdIter begin, FwdIter end,
                                     const block_view<T>& dest) -> typename block_view<T>::iterator
            {
                return std::copy(begin, end, dest.begin());
            }

            template <typename T, class FwdIter>
            raw_pointer copy_or_move_construct(std::true_type, FwdIter begin, FwdIter end,
                                               const memory_block& dest)
            {
                return uninitialized_move_convert<T>(begin, end, dest);
            }
            template <typename T, class FwdIter>
            raw_pointer copy_or_move_construct(std::false_type, FwdIter begin, FwdIter end,
                                               const memory_block& dest)
            {
                return uninitialized_copy_convert<T>(begin, end, dest);
            }

            template <bool Move, class BlockStorage, typename T, typename FwdIter>
            block_view<T> assign_impl(std::integral_constant<bool, Move> move, BlockStorage& dest,
                                      block_view<T> dest_constructed, FwdIter begin, FwdIter end)
            {
                dest_constructed = move_to_front(dest, dest_constructed);

                auto new_size = size_type(std::distance(begin, end)) * sizeof(T);
                auto cur_size = dest_constructed.size() * sizeof(T);
                if (new_size <= cur_size)
                {
                    auto new_end = copy_or_move_assign(move, begin, end, dest_constructed);
                    destroy_range(new_end, dest_constructed.end());
                    return block_view<T>(dest_constructed.begin(), new_end);
                }
                else if (new_size <= dest.block().size())
                {
                    auto assign_end = std::next(begin, std::ptrdiff_t(dest_constructed.size()));
                    copy_or_move_assign(move, begin, assign_end, dest_constructed);
                    auto new_end =
                        copy_or_move_construct<T>(move, assign_end, end,
                                                  memory_block(to_raw_pointer(
                                                                   dest_constructed.data_end()),
                                                               dest.block().end()));
                    return block_view<T>(memory_block(dest.block().begin(), new_end));
                }
                else
                {
                    auto new_begin = clear_and_reserve(dest, dest_constructed, new_size);
                    auto new_end   = copy_or_move_construct<T>(move, begin, end, dest.block());
                    return block_view<T>(memory_block(new_begin, new_end));
                }
            }
        } // namespace detail

        /// \effects Increases the size of the block owned by `dest` to be as big as needed.
        /// then copy constructs or assigns the objects over.
        /// \returns A view to the objects now constructed in `dest`.
        /// \throws Anything thrown by the allocation or copy constructor/assignment of `T`.
        /// \requires The constructed objects must start at the beginning of the memory.
        template <class BlockStorage, typename T, typename FwdIter>
        block_view<T> assign_copy(BlockStorage& dest, block_view<T> dest_constructed, FwdIter begin,
                                  FwdIter end)
        {
            return detail::assign_impl(std::false_type{}, dest, dest_constructed, begin, end);
        }
        template <class BlockStorage, typename T, typename FwdIter>
        block_view<T> assign_move(BlockStorage& dest, block_view<T> dest_constructed, FwdIter begin,
                                  FwdIter end)
        {
            return detail::assign_impl(std::true_type{}, dest, dest_constructed, begin, end);
        }

        /// \effects Increases the size of the block to be at least `n`,
        /// then fills it by copy constructing/assigning `obj`.
        /// \returns A view to the objects now constructed in `dest`.
        /// \throws Anything thrown by the allocation or copy constructor/assignment of `T`.
        /// \requires The constructed objects must start at the beginning of the memory.
        template <class BlockStorage, typename T>
        block_view<T> fill(BlockStorage& dest, block_view<T> dest_constructed, size_type n,
                           const T& obj)
        {
            dest_constructed = move_to_front(dest, dest_constructed);

            auto cur_size = dest_constructed.size();
            if (n <= cur_size)
            {
                auto new_end = std::fill_n(dest_constructed.begin(), std::size_t(n), obj);
                destroy_range(new_end, dest_constructed.end());
                return block_view<T>(dest_constructed.data(), n);
            }
            else if (n * sizeof(T) <= dest.block().size())
            {
                std::fill_n(dest_constructed.begin(), cur_size, obj);
                auto new_end =
                    uninitialized_fill(memory_block(to_raw_pointer(dest_constructed.data_end()),
                                                    dest.block().end()),
                                       n - cur_size, obj);
                return block_view<T>(memory_block(dest.block().begin(), new_end));
            }
            else
            {
                auto new_begin = clear_and_reserve(dest, dest_constructed, n * sizeof(T));
                auto new_end   = uninitialized_fill(dest.block(), n, obj);
                return block_view<T>(memory_block(new_begin, new_end));
            }
        }

        /// Move assignment for block storage.
        /// \effects Transfers ownership of the memory of `other` and objects constructed in it to `dest`,
        /// releasing the memory and objects created in it.
        /// \returns A view on the objects now constructed in `dest`.
        /// It is either the same as `other_constructed` or a view starting at the beginning of the memory now owned by `dest`.
        /// \throws Anything thrown by the copy/move constructor of `T` if actual physical objects need to be moved.
        /// \notes This function will propagate the arguments of the block storage from `other` to `dest`.
        /// This allows taking ownership of the memory allocated by `dest`.
        template <class BlockStorage, typename T>
        block_view<T> move_assign(
            BlockStorage& dest, block_view<T> dest_constructed, BlockStorage&& other,
            block_view<T>
                other_constructed) noexcept(block_storage_nothrow_move<BlockStorage, T>::value)
        {
            // 1. clear the destination
            clear_and_shrink(dest, dest_constructed);
            // dest now owns no memory block

            // 2. swap ownership of the memory blocks
            block_view<T> result(dest.block().resize(0));
            BlockStorage::swap(dest, result, other, other_constructed);

            // other is now empty, other_constructed empty view
            // dest now contains the memory of other
            // result views the objects created in that memory
            return result;
        }

        namespace detail
        {
            template <typename T>
            struct const_block_view
            {
                using type = block_view<const T>;
            };
        } // namespace detail

        /// Copy assignment for block storage.
        /// \effects Allocates new memory using the arguments from `other` and copies the objects over.
        /// Then changes `dest` to own that memory, releasing previously owned memory.
        /// \returns A view on the objects now constructed in `dest`.
        /// It starts at the beginning of the memory now owned by `dest`.
        /// \throws Anything thrown by the allocation function or copy constructor of `T`.
        /// \notes This function is like [array::assign](), but propagates the arguments of the block storage.
        /// This makes it less efficient as memory of `dest` cannot be reused.
        template <class BlockStorage, typename T>
        block_view<T> copy_assign(BlockStorage& dest, block_view<T> dest_constructed,
                                  const BlockStorage&                        other,
                                  typename detail::const_block_view<T>::type other_constructed)
        {
            auto size_in_bytes = other_constructed.size() * sizeof(T);

            // 1. create a copy of the objects in temporary storage
            BlockStorage temp(argument_of(other));
            temp.reserve(size_in_bytes, block_view<T>(temp.block().resize(0u)));
            uninitialized_copy(other_constructed.begin(), other_constructed.end(), temp.block());
            auto temp_constructed = block_view<T>(temp.block().resize(size_in_bytes));
            // if it throws, nothing has changed

            // 2. destroy existing objects
            destroy_range(dest_constructed.begin(), dest_constructed.end());
            dest_constructed = block_view<T>(dest.block().resize(0u));

            // 3. swap temp and destination
            BlockStorage::swap(temp, temp_constructed, dest, dest_constructed);

            return dest_constructed;

            // destructor of temp frees previous memory of dest
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED
