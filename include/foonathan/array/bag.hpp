// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BAG_HPP_INCLUDED
#define FOONATHAN_ARRAY_BAG_HPP_INCLUDED

#include <foonathan/array/detail/swappable.hpp>
#include <foonathan/array/array.hpp>

namespace foonathan
{
    namespace array
    {
        /// A bag of elements.
        ///
        /// It is the most basic collection of elements.
        /// Insert and erase are (amortized) O(1), but the order of elements is not determined.
        ///
        /// It is implemented as a tiny wrapper over [array::array<T>]().
        template <typename T, class BlockStorage = block_storage_default>
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
            bag() = default;

            /// \effects Creates a bag without any elements.
            /// The block storage is initialized with the given arguments.
            explicit bag(argument_type<BlockStorage> arg) noexcept : array_(arg) {}

            /// \effects Creates a bag containing the elements of the view.
            /// The block storage is initialized with the given arguments.
            explicit bag(input_view<T, BlockStorage>&& input, argument_type<BlockStorage> arg = {})
            : array_(std::move(input), arg)
            {
            }

            /// \effects Same as `assign(std::move(view))`.
            bag& operator=(input_view<T, BlockStorage>&& view)
            {
                assign(std::move(view));
                return *this;
            }

            /// Swap.
            friend void swap(bag& lhs,
                             bag& rhs) noexcept(block_storage_nothrow_move<BlockStorage, T>::value)
            {
                swap(lhs.array_, rhs.array_);
            }

            //=== access ===//
            /// \returns A block view to the elements.
            operator block_view<T>() noexcept
            {
                return array_;
            }
            /// \returns A `const` block view to the elements.
            operator block_view<const T>() const noexcept
            {
                return array_;
            }

            /// \returns An input view to the elements.
            operator input_view<T, BlockStorage>() && noexcept
            {
                return std::move(array_).operator input_view<T, BlockStorage>();
            }

            iterator begin() noexcept
            {
                return iterator(iterator_tag{}, iterator_to_pointer(array_.begin()));
            }
            const_iterator begin() const noexcept
            {
                return cbegin();
            }
            const_iterator cbegin() const noexcept
            {
                return const_iterator(iterator_tag{}, iterator_to_pointer(array_.begin()));
            }

            iterator end() noexcept
            {
                return iterator(iterator_tag{}, iterator_to_pointer(array_.end()));
            }
            const_iterator end() const noexcept
            {
                return cend();
            }
            const_iterator cend() const noexcept
            {
                return const_iterator(iterator_tag{}, iterator_to_pointer(array_.end()));
            }

            //=== capacity ===//
            /// \returns Whether or not the bag is empty.
            bool empty() const noexcept
            {
                return array_.empty();
            }

            /// \returns The number of elements in the bag.
            size_type size() const noexcept
            {
                return array_.size();
            }

            /// \returns The number of elements the bag can contain without reserving new memory.
            size_type capacity() const noexcept
            {
                return array_.capacity();
            }

            /// \returns The maximum number of elements as determined by the block storage.
            size_type max_size() const noexcept
            {
                return array_.max_size();
            }

            /// \effects Reserves new memory to make capacity as least as big as `new_capacity` if that isn't the case already.
            void reserve(size_type new_capacity)
            {
                return array_.reserve(new_capacity);
            }

            /// \effects Non-binding request to make the capacity as small as necessary.
            void shrink_to_fit()
            {
                array_.shrink_to_fit();
            }

            //=== modifiers ===//
            /// \effects Creates a new element in the bag by calling `emplace_back()` on the array.
            /// \returns A reference to the constructed element.
            template <typename... Args>
            T& emplace(Args&&... args)
            {
                return array_.emplace_back(std::forward<Args>(args)...);
            }

            /// \effects Same as `emplace(element)`.
            void insert(const T& element)
            {
                array_.push_back(element);
            }
            /// \effects Same as `emplace(std::move(element))`.
            void insert(T&& element)
            {
                array_.push_back(std::move(element));
            }

            /// \effects Same as `insert_range(view.begin(), view.end())`.
            iterator insert(const block_view<const T>& view)
            {
                return insert_range(view.begin(), view.end());
            }

            /// \effects Inserts the elements in the sequence `[begin, end)` by calling `append_range()` on the array.
            /// \returns An iterator to the first inserted element, or `end()` if the sequence was empty.
            /// The elements are guaranteed to be inserted sequentially.
            template <typename InputIt>
            iterator insert_range(InputIt begin, InputIt end)
            {
                return iterator(iterator_tag{},
                                iterator_to_pointer(array_.append_range(begin, end)));
            }

            /// \effects Destroys all elements.
            void clear() noexcept
            {
                array_.clear();
            }

            /// \effects Destroys and removes the element at the given position.
            /// \returns An iterator after the element that was removed.
            iterator erase(const_iterator iter) noexcept(detail::is_nothrow_swappable<T>::value)
            {
                // const_cast is fine, no element was const
                auto ptr = const_cast<T*>(iterator_to_pointer(iter));

                // replace with the last element
                auto ptr_last = &array_.back();
                *ptr          = std::move(*ptr_last);

                // now remove the last element
                array_.pop_back();

                // iterator after the removed element is iter again
                return iterator(iterator_tag{}, ptr);
            }

            /// \effects Destroys all elements in the range `[begin, end)`.
            /// \returns An iterator after the last element that was removed.
            iterator erase_range(const_iterator begin, const_iterator end) noexcept(
                std::is_nothrow_move_assignable<T>::value)
            {
                // again, const_cast is fine
                auto begin_ptr = const_cast<T*>(iterator_to_pointer(begin));
                auto end_ptr   = const_cast<T*>(iterator_to_pointer(end));
                auto count     = end_ptr - begin_ptr;

                auto view = block_view<T>(*this);

                // move the range to the end
                auto position_at_end = [&] {
                    auto distance_from_end = view.data_end() - end_ptr;
                    if (distance_from_end == 0)
                        // already at the end
                        return begin_ptr;
                    else if (distance_from_end <= count)
                    {
                        // move everything after before the elements
                        return std::move(end_ptr, view.data_end(), begin_ptr);
                    }
                    else
                    {
                        // swap the range with elements at the end
                        auto target_end   = view.data_end();
                        auto target_begin = target_end - count;
                        std::swap_ranges(begin_ptr, end_ptr, target_begin);
                        return target_begin;
                    }
                }();

                // remove the elements at the end
                array_.erase_range(pointer_to_iterator<typename array<T, BlockStorage>::iterator>(
                                       position_at_end),
                                   pointer_to_iterator<typename array<T, BlockStorage>::iterator>(
                                       view.data_end()));

                // iterator after the last removed element is begin again
                return iterator(iterator_tag{}, begin_ptr);
            }

            /// \effects Conceptually the same as `*this = bag<T>(block)`.
            void assign(input_view<T, BlockStorage>&& block)
            {
                array_.assign(std::move(block));
            }

            /// \effects Conceptually the same as `bag<T> b; b.insert_range(begin, end); *this = std::move(b);`
            template <typename InputIt>
            void assign_range(InputIt begin, InputIt end)
            {
                array_.assign_range(begin, end);
            }

        private:
            array<T, BlockStorage> array_;
        };

        namespace detail
        {
            template <class Bag>
            struct bag_assign_proxy
            {
                Bag* bag;

                template <typename U>
                auto operator=(U&& arg) -> decltype((void)bag->insert(std::forward<U>(arg)))
                {
                    bag->insert(std::forward<U>(arg));
                }
            };
        } // namespace detail

        /// An `OutputIterator` that can be used to insert into an [array::bag]().
        template <class Bag>
        class bag_insert_iterator
        {
        public:
            using iterator_category = std::output_iterator_tag;
            using value_type        = void;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = void;

            explicit bag_insert_iterator(Bag& bag) : bag_(&bag) {}

            detail::bag_assign_proxy<Bag> operator*() const
            {
                return {bag_};
            }

            bag_insert_iterator& operator++()
            {
                return *this;
            }
            bag_insert_iterator operator++(int)
            {
                return *this;
            }

        private:
            Bag* bag_;
        };

        /// \returns An [array::bag_insert_iterator]() inserting into the given bag.
        template <typename T, class BlockStorage>
        bag_insert_iterator<bag<T, BlockStorage>> bag_inserter(bag<T, BlockStorage>& bag)
        {
            return bag_insert_iterator<foonathan::array::bag<T, BlockStorage>>(bag);
        }
    } // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_BAG_HPP_INCLUDED
