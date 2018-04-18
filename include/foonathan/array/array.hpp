// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_ARRAY_HPP_INCLUDED
#define FOONATHAN_ARRAY_ARRAY_HPP_INCLUDED

#include <foonathan/array/block_storage.hpp>
#include <foonathan/array/array_view.hpp>
#include <foonathan/array/input_view.hpp>
#include <foonathan/array/pointer_iterator.hpp>

namespace foonathan
{
    namespace array
    {
        /// An array of elements.
        ///
        /// This is the `[std::vector]()` implementation, but uses a `BlockStorage`.
        /// \notes It has slight interface differences (improvements),
        /// so is not a drop-in replacement for `std::vector`.
        template <typename T, class BlockStorage>
        class array
        {
        public:
            class iterator_tag
            {
                constexpr iterator_tag() = default;

                friend array;
            };

        public:
            using value_type    = T;
            using block_storage = BlockStorage;

            using iterator       = pointer_iterator<iterator_tag, T>;
            using const_iterator = pointer_iterator<iterator_tag, const T>;

            //=== constructors/destructors ===//
            /// Default constructor.
            /// \effects Creates an array without any elements.
            /// The block storage is initialized with default constructed arguments.
            array() : array(typename block_storage::arg_type{}) {}

            /// \effects Creates an array without any elements.
            /// The block storage is initialized with the given arguments.
            explicit array(typename block_storage::arg_type args) noexcept
            : storage_(std::move(args)), end_(storage_.block().begin())
            {
            }

            /// \effects Creates an array containing the elements of the view.
            /// The block storage is initialized with the given arguments.
            explicit array(input_view<T, BlockStorage>&&    input,
                           typename block_storage::arg_type args = {})
            : array(std::move(args))
            {
                auto new_view = std::move(input).release(storage_, view());
                new_view      = move_to_front(storage_, new_view);
                end_          = new_view.block().end();
            }

            /// Copy constructor.
            array(const array& other) : array(other.storage_.arguments())
            {
                append_range(other.begin(), other.end());
            }

            /// Move constructor.
            array(array&& other) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            : array(other.storage_.arguments())
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
            ~array() noexcept
            {
                destroy_range(begin(), end());
            }

            /// Copy assignment operator.
            array& operator=(const array& other)
            {
                auto new_view = copy_assign(storage_, view(), other.storage_, other.view());
                end_          = new_view.block().end();
                return *this;
            }

            /// Move assignment operator.
            array& operator=(array&& other) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            {
                auto new_view =
                    move_assign(storage_, view(), std::move(other.storage_), other.view());
                end_       = new_view.block().end();
                other.end_ = other.storage_.block().begin();
                return *this;
            }

            /// \effects Same as `assign(std::move(view))`.
            array& operator=(input_view<T, BlockStorage>&& view)
            {
                assign(std::move(view));
                return *this;
            }

            /// Swap.
            friend void swap(array& lhs,
                             array& rhs) noexcept(block_storage_nothrow_move<BlockStorage, T>{})
            {
                auto lhs_view = lhs.view();
                auto rhs_view = rhs.view();
                BlockStorage::swap(lhs.storage_, lhs_view, rhs.storage_, rhs_view);
                lhs.end_ = lhs_view.block().end();
                rhs.end_ = rhs_view.block().end();
            }

            //=== access ===//
            /// \returns An array view to the elements.
            operator array_view<T>() noexcept
            {
                return view();
            }
            /// \returns A `const` array view to the elements.
            operator array_view<const T>() const noexcept
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

            T& operator[](size_type i) noexcept
            {
                return view()[i];
            }
            const T& operator[](size_type i) const noexcept
            {
                return view()[i];
            }

            T& front() noexcept
            {
                return view().front();
            }
            const T& front() const noexcept
            {
                return view().front();
            }

            T& back() noexcept
            {
                return view().back();
            }
            const T& back() const noexcept
            {
                return view().back();
            }

            //=== capacity ===//
            /// \returns Whether or not the array is empty.
            bool empty() const noexcept
            {
                return view().empty();
            }

            /// \returns The number of elements in the array.
            size_type size() const noexcept
            {
                return view().size();
            }

            /// \returns The number of elements the array can contain without reserving new memory.
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
            /// \effects Same as a call to `emplace(end, args...)`.
            /// \returns A reference to the newly constructed element.
            template <typename... Args>
            T& emplace_back(Args&&... args)
            {
                reserve_impl(sizeof(T));
                auto ptr = construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
                return *ptr;
            }

            /// \effects Same as `emplace_back(element)`.
            void push_back(const T& element)
            {
                emplace_back(element);
            }

            /// \effects Same as `emplace_back(std::move(element))`.
            void push_back(T&& element)
            {
                emplace_back(std::move(element));
            }

            /// \effects Creates a new element before the specified iterator.
            /// \returns An iterator to the element that was just inserted.
            template <typename... Args>
            iterator emplace(const_iterator pos, Args&&... args)
            {
                // note: we emplace at a given position by creating the objects at the back,
                // then rotating it to the correct position
                //
                // when the capacity is sufficient, this is almost as efficient as possible,
                // just one extra swap or so
                //
                // when the capacity isn't sufficient, the block storage could in theory leave a hole already,
                // however this would vastly complicate the interface and just isn't worth it,
                // as it is the slow path anyway

                auto index = pos - begin();
                emplace_back(std::forward<Args>(args)...);
                std::rotate(begin() + index, std::prev(end()), end());
                return begin() + index;
            }

            /// \effects Same as `emplace(pos, element)`.
            iterator insert(const_iterator pos, const T& element)
            {
                return emplace(pos, element);
            }

            /// \effects Same as `emplace(pos, std::move(element))`.
            iterator insert(const_iterator pos, T&& element)
            {
                return emplace(pos, std::move(element));
            }

            /// \effects Same as `append_range(block.begin(), block.end())`.
            iterator append(array_view<const T> block)
            {
                return append_range(block.begin(), block.end());
            }

            /// \effects Same as `insert_range(end(), begin, end)`.
            template <typename InputIt>
            iterator append_range(InputIt begin, InputIt end)
            {
                return append_range_impl(typename std::iterator_traits<
                                             InputIt>::iterator_category{},
                                         begin, end);
            }

            /// \effects Same as `insert_range(pos, block.begin(), block.end())`.
            iterator insert(const_iterator pos, array_view<const T> block)
            {
                return insert_range(pos, block.begin(), block.end());
            }

            /// \effects Inserts elements from the range `[begin, end)` before `pos`.
            /// \returns An iterator to the first inserted element, or `pos` if the range was empty.
            template <typename InputIt>
            iterator insert_range(const_iterator pos, InputIt begin, InputIt end)
            {
                return insert_range_impl(typename std::iterator_traits<
                                             InputIt>::iterator_category{},
                                         begin, end);
            }

            /// \effects Destroys all elements.
            void clear() noexcept
            {
                destroy_range(begin(), end());
                end_ = storage_.block().begin();
            }

            /// \effects Same as `erase(std::prev(end())`.
            void pop_back() noexcept
            {
                end_ = destroy_object(&*std::prev(end()));
            }

            /// \effects Destroys and removes the element at the given position.
            /// \returns An iterator after the element that was removed.
            iterator erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable<T>::value)
            {
                auto mut_pos = const_cast<T*>(iterator_to_pointer(pos));

                // move all elements after to the front
                std::move(std::next(mut_pos), view().data_end(), mut_pos);
                // destroy the last element
                pop_back();

                // next element after is at the location of pos
                return iterator(iterator_tag{}, mut_pos);
            }

            /// \effects Destroys and removes all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            iterator erase_range(const_iterator begin, const_iterator end) noexcept(
                std::is_nothrow_move_assignable<T>::value)
            {
                auto mut_begin = const_cast<T*>(iterator_to_pointer(begin));
                auto mut_end   = const_cast<T*>(iterator_to_pointer(end));

                // move all elements after to the front
                std::move(mut_end, view().data_end(), mut_begin);

                // destroy the elements at the end
                auto n = mut_end - mut_begin;
                destroy_range(std::prev(view().data_end(), n), view().data_end());
                end_ -= n * sizeof(T);

                // next element after is still the first location of the range
                return iterator(iterator_tag{}, mut_begin);
            }

            /// \effects Conceptually the same as `*this = array<T>(block)`.
            void assign(input_view<T, BlockStorage>&& block)
            {
                auto new_view = std::move(block).release(storage_, view());
                new_view      = move_to_front(storage_, new_view);
                end_          = new_view.block().end();
            }

            /// \effects Conceptually the same as `array<T> a; a.insert_range(begin, end); *this = std::move(a);`
            template <typename InputIt>
            void assign_range(InputIt begin, InputIt end)
            {
                auto new_view = assign_copy(storage_, view(), begin, end);
                end_          = new_view.block().end();
            }

        private:
            array_view<T> view() const noexcept
            {
                return array_view<T>(to_pointer<T>(storage_.block().begin()), to_pointer<T>(end_));
            }

            void reserve_impl(size_type min_additional_bytes)
            {
                auto cur_size_bytes = size_type(end_ - storage_.block().begin());
                auto new_size_bytes = cur_size_bytes + min_additional_bytes;

                if (new_size_bytes > storage_.block().size())
                    end_ = storage_.reserve(min_additional_bytes, view());
            }

            template <typename InputIt>
            iterator append_range_impl(std::input_iterator_tag, InputIt begin, InputIt end)
            {
                auto iter = this->end();
                for (auto cur = begin; cur != end; ++cur)
                {
                    push_back(*cur);
                    iter = std::prev(this->end());
                }
                return iter;
            }
            template <typename ForwardIt>
            iterator append_range_impl(std::forward_iterator_tag, ForwardIt begin, ForwardIt end)
            {
                auto needed = size_type(std::distance(begin, end));
                reserve_impl(needed * sizeof(T));

                auto iter = this->end();
                for (auto cur = begin; cur != end; ++cur)
                {
                    construct_object<T>(end_, *cur);
                    end_ += sizeof(T);
                }
                return iter;
            }

            template <typename InputIt>
            iterator insert_range_impl(const_iterator pos, std::input_iterator_tag, InputIt begin,
                                       InputIt end)
            {
                auto iter = this->end();
                for (auto cur = begin; cur != end; ++cur)
                {
                    pos  = insert(pos, *cur);
                    iter = std::prev(this->end());
                    ++pos;
                }
                return iter;
            }
            template <typename ForwardIt>
            iterator insert_range_impl(const_iterator pos, std::forward_iterator_tag,
                                       ForwardIt begin, ForwardIt end)
            {
                auto index = pos - this->begin();

                // again, like with insert, do an append, plus rotate
                auto new_begin = append_range(begin, end);
                std::rotate(this->begin() + index, new_begin, this->end());

                return this->begin() + index;
            }

            BlockStorage storage_;
            raw_pointer  end_;
        };
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_ARRAY_HPP_INCLUDED
