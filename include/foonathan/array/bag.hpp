// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BAG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BAG_HPP_INCLUDED

#include <foonathan/array/block_storage.hpp>
#include <foonathan/array/block_view.hpp>
#include <foonathan/array/input_view.hpp>
#include <foonathan/array/pointer_iterator.hpp>
#include <foonathan/array/raw_storage.hpp>

namespace foonathan
{
    namespace array
    {
        /// A bag of elements.
        ///
        /// It is the most basic collection of elements.
        /// Insert and erase are (amortized) O(1), but the order of elements is not determined.
        template <typename T, class BlockStorage>
        class bag
        {
            class iterator_tag
            {
                constexpr iterator_tag() = default;

                friend bag;
            };

        public:
            using value_type    = T;
            using block_storage = BlockStorage;

            using iterator       = pointer_iterator<iterator_tag, T>;
            using const_iterator = pointer_iterator<iterator_tag, const T>;

            //=== constructors/destructors ===//
            /// Default constructor.
            /// \effects Creates a bag without any elements.
            /// The block storage is initialized with default constructed arguments.
            bag() : bag(typename block_storage::arg_type{}) {}

            /// \effects Creates a bag without any elements.
            /// The block storage is initialized with the given arguments.
            explicit bag(typename block_storage::arg_type args) noexcept
            : storage_(std::move(args)), end_(storage_.block().begin())
            {
            }

            /// \effects Creates a bag containing the elements of the view.
            /// The block storage is initialized with the given arguments.
            explicit bag(input_view<T, BlockStorage>&&    input,
                         typename block_storage::arg_type args = {})
            : bag(std::move(args))
            {
                auto new_view = std::move(input).release(storage_, view());
                new_view      = move_to_front(storage_, new_view);
                end_          = new_view.block().end();
            }

            /// Copy constructor.
            bag(const bag& other) : bag(other.storage_.arguments())
            {
                insert_range(other.begin(), other.end());
            }

            /// Move constructor.
            bag(bag&& other) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            : bag(other.storage_.arguments())
            {
                // swap the owned blocks
                auto my_view    = view();
                auto other_view = other.view();
                BlockStorage::swap(storage_, my_view, other.storage_, other_view);
                assert(other_view.empty());

                // update the end pointer
                end_       = my_view.block().end();
                other.end_ = other_view.block().end();
            }

            /// Destructor.
            ~bag() noexcept
            {
                destroy_range(begin(), end());
            }

            /// Copy assignment operator.
            bag& operator=(const bag& other)
            {
                auto new_view = copy_assign(storage_, view(), other.storage_, other.view());
                end_          = new_view.block().end();
                return *this;
            }

            /// Move assignment operator.
            bag& operator=(bag&& other) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            {
                auto new_view =
                    move_assign(storage_, view(), std::move(other.storage_), other.view());
                end_       = new_view.block().end();
                other.end_ = other.storage_.block().begin();
                return *this;
            }

            /// \effects Same as `assign(std::move(view))`.
            bag& operator=(input_view<T, BlockStorage>&& view)
            {
                assign(std::move(view));
                return *this;
            }

            /// Swap.
            friend void swap(bag& lhs,
                             bag& rhs) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            {
                auto lhs_view = lhs.view();
                auto rhs_view = rhs.view();
                BlockStorage::swap(lhs.storage_, lhs_view, rhs.storage_, rhs_view);
                lhs.end_ = lhs_view.block().end();
                rhs.end_ = rhs_view.block().end();
            }

            //=== access ===//
            /// \returns A block view to the elements.
            operator block_view<T>() noexcept
            {
                return view();
            }
            /// \returns A `const` block view to the elements.
            operator block_view<const T>() const noexcept
            {
                return view();
            }

            /// \returns An input view to the elements.
            operator input_view<T, BlockStorage>() && noexcept
            {
                auto result = input_view<T, BlockStorage>(std::move(storage_), view());
                end_        = storage_.empty_block().begin();
                return result;
            }

            iterator begin() noexcept
            {
                return iterator(iterator_tag{}, view().data());
            }
            const_iterator begin() const noexcept
            {
                return cbegin();
            }
            const_iterator cbegin() const noexcept
            {
                return const_iterator(iterator_tag{}, view().data());
            }

            iterator end() noexcept
            {
                return iterator(iterator_tag{}, view().data_end());
            }
            const_iterator end() const noexcept
            {
                return cend();
            }
            const_iterator cend() const noexcept
            {
                return const_iterator(iterator_tag{}, view().data_end());
            }

            //=== capacity ===//
            /// \returns Whether or not the bag is empty.
            bool empty() const noexcept
            {
                return view().empty();
            }

            /// \returns The number of elements in the bag.
            size_type size() const noexcept
            {
                return view().size();
            }

            /// \returns The number of elements the bag can contain without reserving new memory.
            size_type capacity() const noexcept
            {
                return size_type(to_pointer<T>(storage_.block().end())
                                 - to_pointer<T>(storage_.block().begin()));
            }

            /// \returns The maximum number of elements as determined by the block storage.
            size_type max_size() const noexcept
            {
                return BlockStorage::max_size(storage_.arguments()) / sizeof(T);
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity` if that isn't the case already.
            void reserve(size_type new_capacity)
            {
                auto new_cap_bytes = new_capacity * sizeof(T);
                if (new_cap_bytes > storage_.block().size())
                    reserve_impl(new_cap_bytes - storage_.block().size());
            }

            /// \effects Non-binding request to make the capacity as small as necessary.
            void shrink_to_fit()
            {
                end_ = storage_.shrink_to_fit(view());
            }

            //=== modifiers ===//
            /// \effects Creates a new element in the bag.
            template <typename... Args>
            void emplace(Args&&... args)
            {
                reserve_impl(sizeof(T));
                construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
            }

            /// \effects Same as `emplace(value)`.
            void insert(const T& value)
            {
                emplace(value);
            }
            /// \effects Same as `emplace(std::move(value))`.
            void insert(T&& value)
            {
                emplace(std::move(value));
            }

            /// \effects Same as `insert_range(view.begin(), view.end())`.
            void insert_block(const block_view<const T>& view)
            {
                insert_range(view.begin(), view.end());
            }

            /// \effects Inserts the elements in the sequence `[begin, end)`.
            template <typename InputIt>
            void insert_range(InputIt begin, InputIt end)
            {
                insert_range_impl(typename std::iterator_traits<InputIt>::iterator_category{},
                                  begin, end);
            }

            /// \effects Destroys all elements.
            void clear() noexcept
            {
                destroy_range(begin(), end());
                end_ = storage_.block().begin();
            }

            /// \effects Destroys the element at the given position.
            /// \returns An iterator after the element that was removed.
            iterator erase(const_iterator iter) noexcept(std::is_nothrow_swappable<T>{})
            {
                // const_cast is fine, no element was const
                auto ptr = const_cast<T*>(iterator_to_pointer(iter));

                // swap with the last element, if it is not already last
                auto ptr_last = std::prev(view().data_end());
                if (ptr != ptr_last)
                    std::iter_swap(ptr, ptr_last);

                // now remove the last element
                end_ = destroy_object(ptr_last);

                // iterator after the removed element is iter again
                return iterator(iterator_tag{}, ptr);
            }

            /// \effects Destroys all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            iterator erase_range(const_iterator begin,
                                 const_iterator end) noexcept(std::is_nothrow_move_assignable<T>{})
            {
                // again, const_cast is fine
                auto begin_ptr = const_cast<T*>(iterator_to_pointer(begin));
                auto end_ptr   = const_cast<T*>(iterator_to_pointer(end));
                auto count     = end_ptr - begin_ptr;

                // move the range to the end
                auto position_at_end = [&] {
                    auto distance_from_end = view().data_end() - end_ptr;
                    if (distance_from_end == 0)
                        // already at the end
                        return begin_ptr;
                    else if (distance_from_end <= count)
                    {
                        // move everything after before the elements
                        return std::move(end_ptr, view().data_end(), begin_ptr);
                    }
                    else
                    {
                        // swap the range with elements at the end
                        auto target_end   = view().data_end();
                        auto target_begin = target_end - count;
                        std::swap_ranges(begin_ptr, end_ptr, target_begin);
                        return target_begin;
                    }
                }();

                // remove the elements at the end
                destroy_range(position_at_end, view().data_end());
                end_ = as_raw_pointer(position_at_end);

                // iterator after the last removed element is begin again
                return iterator(iterator_tag{}, begin_ptr);
            }

            /// \effects Conceptually the same as `*this = bag<T>(block)`.
            void assign(input_view<T, BlockStorage>&& block)
            {
                auto new_view = std::move(block).release(storage_, view());
                new_view      = move_to_front(storage_, new_view);
                end_          = new_view.block().end();
            }

            /// \effects Conceptually the same as `bag<T> b; b.insert_range(begin, end); *this = std::move(b);`
            template <typename InputIt>
            void assign_range(InputIt begin, InputIt end)
            {
                auto new_view = assign_copy(storage_, view(), begin, end);
                end_          = new_view.block().end();
            }

        private:
            block_view<T> view() const noexcept
            {
                return block_view<T>(to_pointer<T>(storage_.block().begin()), to_pointer<T>(end_));
            }

            void reserve_impl(size_type min_additional_bytes)
            {
                auto cur_size_bytes = size_type(end_ - storage_.block().begin());
                auto new_size_bytes = cur_size_bytes + min_additional_bytes;

                if (new_size_bytes > storage_.block().size())
                    end_ = storage_.reserve(min_additional_bytes, view());
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
                auto needed = size_type(std::distance(begin, end));
                reserve_impl(needed * sizeof(T));

                for (auto cur = begin; cur != end; ++cur)
                {
                    construct_object<T>(end_, *cur);
                    end_ += sizeof(T);
                }
            }

            BlockStorage storage_;
            raw_pointer  end_;
        };
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BAG_HPP_INCLUDED
