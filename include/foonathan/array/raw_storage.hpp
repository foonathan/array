// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_RAW_STORAGE_HPP_INCLUDED
#define FOONATHAN_ARRAY_RAW_STORAGE_HPP_INCLUDED

#include <iterator>
#include <new>
#include <type_traits>

#include <foonathan/array/memory_block.hpp>

namespace foonathan
{
    namespace array
    {
        /// \effects Creates a new object at the given location.
        /// \returns A pointer to the newly created object.
        template <typename T, typename... Args>
        T* construct_object(raw_pointer ptr, Args&&... args)
        {
            return ::new (to_void_pointer(ptr)) T(std::forward<Args>(args)...);
        }

        /// \effects Destroys an object at the given location.
        template <typename T>
        raw_pointer destroy_object(T* object) noexcept
        {
            object->~T();
            return from_pointer(object);
        }

        /// A RAII object to manage created objects in a range.
        template <typename T>
        class partially_constructed_range
        {
        public:
            /// \effects Creates it giving it the start address.
            explicit partially_constructed_range(raw_pointer memory)
            : begin_(to_pointer<T>(memory)), end_(memory)
            {
            }

            /// \effects Creates it giving it the memory block it uses to create the objects.
            explicit partially_constructed_range(const memory_block& block)
            : partially_constructed_range(block.memory)
            {
            }

            /// \effects Creates it giving it the memory block
            /// and a range of objects that have already been created.
            partially_constructed_range(const memory_block& block, raw_pointer constructed_end)
            : begin_(to_pointer<T>(block.memory)), end_(constructed_end)
            {
            }

            partially_constructed_range(const partially_constructed_range&) = delete;
            partially_constructed_range& operator=(const partially_constructed_range&) = delete;

            /// \effects Destroys all objects already created.
            ~partially_constructed_range() noexcept
            {
                auto end = to_pointer<T>(end_);
                for (auto cur = begin_; cur != end; ++cur)
                    destroy_object(cur);
            }

            /// \effects Creates a new object at the end.
            /// \returns A pointer to the created object.
            /// \notes Does not check whether there is enough memory left.
            template <typename... Args>
            T* construct_object(Args&&... args)
            {
                auto result = array::construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
                return result;
            }

            /// \effects Releases ownership over the created objects.
            /// \returns A pointer to the next free memory space.
            raw_pointer release() && noexcept
            {
                auto result = end_;
                begin_      = nullptr;
                end_        = nullptr;
                return result;
            }

        private:
            T*          begin_;
            raw_pointer end_;
        };

        /// \effects Moves elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, some objects in the original range may already be in the moved-from state,
        /// however all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_move(InputIter begin, InputIter end, const memory_block& block)
        {
            partially_constructed_range<typename std::iterator_traits<InputIter>::value_type> range(
                block);
            for (auto cur = begin; cur != end; ++cur)
                range.construct_object(std::move(*cur));
            return std::move(range).release();
        }

        /// \effects [std::move_if_noexcept]() elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, the old range has not been modified and all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_move_if_noexcept(InputIter begin, InputIter end,
                                                   const memory_block& block)
        {
            partially_constructed_range<typename std::iterator_traits<InputIter>::value_type> range(
                block);
            for (auto cur = begin; cur != end; ++cur)
                range.construct_object(std::move_if_noexcept(*cur));
            return std::move(range).release();
        }

        /// \effects Copies elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, the old range has not been modified and all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_copy(InputIter begin, InputIter end, const memory_block& block)
        {
            partially_constructed_range<typename std::iterator_traits<InputIter>::value_type> range(
                block);
            for (auto cur = begin; cur != end; ++cur)
                range.construct_object(*cur);
            return std::move(range).release();
        }

        /// \effects Destroys all objects in the given range.
        template <typename FwdIter>
        void destroy_range(FwdIter begin, FwdIter end) noexcept
        {
            using type = typename std::iterator_traits<FwdIter>::value_type;
            for (auto cur = begin; cur != end; ++cur)
                cur->~type();
        }

        /// \effects [std::move_if_noexcept]() elements of the given range to the uninitialized memory of the given block,
        /// then destroys them at the old location.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, the old range has not been modified and all objects created at the new location will be destroyed.
        template <typename FwdIter>
        raw_pointer uninitialized_destructive_move(FwdIter begin, FwdIter end,
                                                   const memory_block& block)
        {
            auto result = uninitialized_move_if_noexcept(begin, end, block);
            destroy_range(begin, end);
            return result;
        }
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_RAW_STORAGE_HPP_INCLUDED
