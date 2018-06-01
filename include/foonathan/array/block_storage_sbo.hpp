// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_SBO_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_SBO_HPP_INCLUDED

#include <cassert>

#include <foonathan/array/block_storage_embedded.hpp>

namespace foonathan
{
    namespace array
    {
        /// A `BlockStorage` that has a small buffer of the given size it uses for small allocations,
        /// then uses the `BigBlockStorage` for dynamic allocations.
        template <std::size_t SmallBufferBytes, class BigBlockStorage>
        class block_storage_sbo : argument_storage_for<BigBlockStorage>
        {
            static_assert(!embedded_storage<BigBlockStorage>::value,
                          "BigBlockStorage must never embedded objects");

        public:
            using embedded_storage = std::true_type;
            using argument_type    = foonathan::array::argument_type<BigBlockStorage>;

            static constexpr std::size_t small_buffer_size =
                SmallBufferBytes < sizeof(BigBlockStorage) ? sizeof(BigBlockStorage) :
                                                             SmallBufferBytes;

            //=== constructors/destructors ===//
            explicit block_storage_sbo(const argument_type& arg) noexcept
            : argument_storage_for<BigBlockStorage>(arg), storage_({})
            {
                // start out small
                block_ = storage_.block();
            }

            block_storage_sbo(const block_storage_sbo&) = delete;
            block_storage_sbo& operator=(const block_storage_sbo&) = delete;

            ~block_storage_sbo() noexcept
            {
                if (is_big())
                    // need to destroy the big storage object
                    destroy_object(&big_storage());
            }

            template <typename T>
            static void swap(block_storage_sbo& lhs, block_view<T>& lhs_constructed,
                             block_storage_sbo& rhs, block_view<T>& rhs_constructed)
            {
                if (lhs.is_small() && rhs.is_small())
                {
                    // forward to small storage
                    block_storage_embedded<small_buffer_size>::swap(lhs.storage_, lhs_constructed,
                                                                    rhs.storage_, rhs_constructed);
                    assert(lhs.block_.begin() == lhs.storage_.block().begin());
                    assert(rhs.block_.begin() == rhs.storage_.block().begin());

                    // propagate stored arguments as well
                    lhs.swap_argument(rhs);
                }
                else if (lhs.is_big() && rhs.is_big())
                {
                    // forward to big storage
                    BigBlockStorage::swap(lhs.big_storage(), lhs_constructed, rhs.big_storage(),
                                          rhs_constructed);
                    lhs.block_ = lhs.big_storage().block();
                    rhs.block_ = rhs.big_storage().block();

                    // propagate stored arguments as well
                    lhs.swap_argument(rhs);
                }
                else if (lhs.is_small())
                    swap_small_big(lhs, lhs_constructed, rhs, rhs_constructed);
                else
                    swap_small_big(rhs, rhs_constructed, lhs, lhs_constructed);
            }

            //=== reserve/shrink_to_fit ===//
            template <typename T>
            void reserve(size_type min_additional_bytes, const block_view<T>& constructed)
            {
                auto new_min_size = block().size() + min_additional_bytes;
                if (could_be_small(new_min_size))
                {
                    if (is_big())
                        // if - for some reason - it was big, make it small
                        transfer_to_small(constructed);
                    else
                        // do a shrink_to_fit to move the elements to the front
                        // (we could also do reserve() but there we have an unnecessary overflow check)
                        storage_.shrink_to_fit(constructed);
                }
                else if (is_small())
                    // transfer elements from the small storage to the big storage
                    transfer_to_big(new_min_size, constructed);
                else
                {
                    // make the big storage even bigger
                    big_storage().reserve(min_additional_bytes, constructed);
                    block_ = big_storage().block();
                }
            }

            template <typename T>
            void shrink_to_fit(const block_view<T>& constructed)
            {
                if (is_small())
                    // make the small storage even smaller
                    // (this only moves the elements to the front)
                    storage_.shrink_to_fit(constructed);
                else if (could_be_small(constructed.size() * sizeof(T)))
                    // transfer elements into the small buffer
                    transfer_to_small(constructed);
                else
                {
                    // forward to the big storage
                    big_storage().shrink_to_fit(constructed);
                    block_ = big_storage().block();
                }
            }

            //=== accessors ===//
            const memory_block& block() const noexcept
            {
                return block_;
            }

            auto argument() const noexcept -> decltype(this->stored_argument())
            {
                return this->stored_argument();
            }

            static size_type max_size(const argument_type& arg) noexcept
            {
                return foonathan::array::max_size<BigBlockStorage>(arg);
            }

        private:
            // if it returns true, storage_ contains the actual objects
            // if it returns false, storage_ contains the BigBlockStorage
            bool is_small() const noexcept
            {
                assert(block_.size() >= small_buffer_size);
                return block_.size() == small_buffer_size;
            }
            bool is_big() const noexcept
            {
                return !is_small();
            }

            BigBlockStorage& big_storage() const noexcept
            {
                assert(is_big());
                return *to_pointer<BigBlockStorage>(storage_.block().begin());
            }

            bool could_be_small(size_type size_in_bytes) const noexcept
            {
                return size_in_bytes <= small_buffer_size;
            }

            // precondition: we have a big empty buffer
            // the elements previously in that buffer are now in the buffer managed by temp
            // we destructive move new_elements into the small buffer
            // if this fails, restore the big buffer as it was before
            template <typename T>
            block_view<T> create_small_buffer(BigBlockStorage& temp, block_view<T> temp_constructed,
                                              const block_view<T>& new_elements)
            {
                assert(is_big() && big_storage().block().empty());

                // destroy the big storage
                destroy_object(&big_storage());

                raw_pointer new_end;
                try
                {
                    // move the elements over into the small buffer
                    new_end = uninitialized_destructive_move(new_elements.begin(),
                                                             new_elements.end(), storage_.block());
                }
                catch (...)
                {
                    // if the move fails, restore the big storage
                    construct_object<BigBlockStorage>(storage_.block().begin(), argument());
                    block_view<T> big_constructed;
                    BigBlockStorage::swap(temp, temp_constructed, big_storage(), big_constructed);
                    throw;
                }

                // elements successfully transferred, finalize by updating block
                block_ = storage_.block();
                return block_view<T>(memory_block(block_.begin(), new_end));
            }

            template <typename T>
            void transfer_to_small(block_view<T> constructed)
            {
                assert(is_big());

                // first we need to transfer ownership of the heap buffer
                // note: this is cheap and nothrow, as it only does pointer swaps, not elements
                BigBlockStorage temp(argument());
                block_view<T>   temp_constructed;
                BigBlockStorage::swap(temp, temp_constructed, big_storage(), constructed);

                // now big_storage() is empty and the memory is owned by temp
                // so create a small buffer containing the temporary elements
                create_small_buffer(temp, temp_constructed, temp_constructed);
            }

            template <typename T>
            void transfer_to_big(size_type new_min_size, const block_view<T>& constructed)
            {
                assert(is_small());

                // first we need a temporary owner for the new memory block
                BigBlockStorage temp(argument());
                temp.reserve(new_min_size, block_view<T>());

                // then we move the elements into the temporary owner
                auto          new_end = uninitialized_destructive_move(constructed.begin(),
                                                              constructed.end(), temp.block());
                block_view<T> temp_constructed(memory_block(temp.block().begin(), new_end));
                // if anything up to this point throws, we haven't changed anything

                // the embedded storage is now free, create the final big storage
                auto big_storage =
                    construct_object<BigBlockStorage>(storage_.block().begin(), argument());

                // transfer ownership to the big storage
                // note: this is cheap and nothrow, as it only does pointer swaps, not elements
                block_view<T> big_constructed;
                BigBlockStorage::swap(*big_storage, big_constructed, temp, temp_constructed);

                // elements successfully transferred, finalize by updating block
                block_ = big_storage->block();
            }

            template <typename T>
            static void swap_small_big(block_storage_sbo& small, block_view<T>& small_constructed,
                                       block_storage_sbo& big, block_view<T>& big_constructed)
            {
                assert(small.is_small() && big.is_big());

                // again, need a temporary owner for the heap memory block
                // this is again, cheap and nothrow
                BigBlockStorage temp(big.argument());
                block_view<T>   temp_constructed;
                BigBlockStorage::swap(temp, temp_constructed, big.big_storage(), big_constructed);

                // big is now empty
                // temp contains the arguments previously owned by big
                // so we can switch big to a small buffer
                big_constructed =
                    big.create_small_buffer(temp, temp_constructed, small_constructed);
                // also propagate the stored arguments
                big.set_stored_argument(small.argument());

                // if we are at this point, big is a copy of the old small
                // and small is now empty
                small_constructed = block_view<T>();

                // create a new big storage in small and transfer the memory owned by temp
                // again, this is cheap and nothrow
                auto small_big_storage =
                    construct_object<BigBlockStorage>(small.storage_.block().begin(),
                                                      argument_of(temp));
                BigBlockStorage::swap(temp, temp_constructed, *small_big_storage,
                                      small_constructed);
                // finalize small by updating its block...
                small.block_ = small_big_storage->block();
                // ... and propagating the stored arguments
                small.set_stored_argument(temp.argument());

                assert(small.is_big() && big.is_small());
            }

            memory_block                              block_; // to avoid branches
            block_storage_embedded<small_buffer_size> storage_;
        };
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_SBO_HPP_INCLUDED
