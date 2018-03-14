// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_RAW_STORAGE_HPP_INCLUDED
#define FOONATHAN_ARRAY_RAW_STORAGE_HPP_INCLUDED

#include <cstring>
#include <iterator>
#include <new>
#include <type_traits>

#include <foonathan/array/contiguous_iterator.hpp>
#include <foonathan/array/memory_block.hpp>

namespace foonathan
{
    namespace array
    {
        /// \effects Creates a new object at the given location using default initialization.
        /// \returns A pointer to the newly created object.
        /// \notes Default initialization may not do any initialization at all.
        template <typename T>
        T* default_construct_object(raw_pointer ptr)
        {
            return ::new (to_void_pointer(ptr)) T;
        }

        /// \effects Creates a new object at the given location using value initialization.
        /// \returns A pointer to the newly created object.
        /// \notes Value initialization will always do zero initialization, even for types without default constructors.
        template <typename T>
        T* value_construct_object(raw_pointer ptr)
        {
            return ::new (to_void_pointer(ptr)) T();
        }

        /// \effects Creates a new object at the given location using `T(std::forward<Args>(args)...)`.
        /// \returns A pointer to the newly created object.
        template <typename T, typename... Args>
        T* paren_construct_object(raw_pointer ptr, Args&&... args)
        {
            return ::new (to_void_pointer(ptr)) T(std::forward<Args>(args)...);
        }

        /// \effects Creates a new object at the given location using `T{std::forward<Args>(args)...}`.
        /// \returns A pointer to the newly created object.
        template <typename T, typename... Args>
        T* list_construct_object(raw_pointer ptr, Args&&... args)
        {
            return ::new (to_void_pointer(ptr)) T{std::forward<Args>(args)...};
        }

        namespace detail
        {
#if defined(__cpp_lib_is_aggregate)
            template <typename T, typename... Args>
            struct use_list_construct : std::is_aggregate<T>
            {
            };
#else
            template <typename T, typename... Args>
            struct use_list_construct
            : std::integral_constant<bool, !std::is_constructible<T, Args...>::value>
            {
            };
#endif

            template <typename T, typename... Args>
            T* construct_object_impl(std::true_type, raw_pointer ptr, Args&&... args)
            {
                return list_construct_object<T>(ptr, std::forward<Args>(args)...);
            }

            template <typename T, typename... Args>
            T* construct_object_impl(std::false_type, raw_pointer ptr, Args&&... args)
            {
                return paren_construct_object<T>(ptr, std::forward<Args>(args)...);
            }
        } // namespace detail

        /// \effects Creates a new object at the given location using `paren_construct_object()`,
        /// unless it is an aggregate, then it uses `list_construct_object()`.
        /// \returns A pointer to the newly created object.
        /// \notes `std::is_aggregate` is only available in C++17 or higher,
        /// before that it tries to use list initialization whenever the other one doesn't compile.
        template <typename T, typename... Args>
        T* construct_object(raw_pointer ptr, Args&&... args)
        {
            return detail::construct_object_impl<T>(detail::use_list_construct<T, Args...>{}, ptr,
                                                    std::forward<Args>(args)...);
        }

        /// \effects Destroys an object at the given location.
        template <typename T>
        raw_pointer destroy_object(T* object) noexcept
        {
            object->~T();
            return from_pointer(object);
        }

        namespace detail
        {
            template <typename T, typename FwdIter>
            void destroy_range(std::true_type, FwdIter, FwdIter) noexcept
            {
                // trivially destructible, no need to do anything
            }

            template <typename T, typename FwdIter>
            void destroy_range(std::false_type, FwdIter begin, FwdIter end) noexcept
            {
                for (auto cur = begin; cur != end; ++cur)
                    cur->~T();
            }
        } // namespace detail

        /// \effects Destroys all objects in the given range.
        template <typename FwdIter>
        void destroy_range(FwdIter begin, FwdIter end) noexcept
        {
            using type = typename std::iterator_traits<FwdIter>::value_type;
            detail::destroy_range<type>(std::is_trivially_destructible<type>{}, begin, end);
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

            /// \effects Creates it giving it the memory block it uses to create the objects in.
            explicit partially_constructed_range(const memory_block& block)
            : partially_constructed_range(block.begin())
            {
            }

            /// \effects Creates it giving it a range of already created objects.
            /// It will create new objects starting at end.
            partially_constructed_range(raw_pointer constructed_begin, raw_pointer constructed_end)
            : begin_(to_pointer<T>(constructed_begin)), end_(constructed_end)
            {
            }

            /// \effects Creates it giving it the memory block
            /// and a range of objects that have already been created.
            partially_constructed_range(const memory_block& block, raw_pointer constructed_end)
            : partially_constructed_range(block.begin(), constructed_end)
            {
            }

            partially_constructed_range(const partially_constructed_range&) = delete;
            partially_constructed_range& operator=(const partially_constructed_range&) = delete;

            /// \effects Destroys all objects already created.
            ~partially_constructed_range() noexcept
            {
                destroy_range(begin_, to_pointer<T>(end_));
            }

            /// \effects Creates a new object at the end using the corresponding free function.
            /// \returns A pointer to the created object.
            /// \notes Does not check whether there is enough memory left.
            /// \group construct
            template <typename... Args>
            T* construct_object(Args&&... args)
            {
                auto result = array::construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
                return result;
            }

            /// \group construct
            template <typename... Args>
            T* brace_construct_object(Args&&... args)
            {
                auto result = array::list_construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
                return result;
            }

            /// \group construct
            template <typename... Args>
            T* paren_construct_object(Args&&... args)
            {
                auto result = array::paren_construct_object<T>(end_, std::forward<Args>(args)...);
                end_ += sizeof(T);
                return result;
            }

            /// \group construct
            T* default_construct_object()
            {
                auto result = array::default_construct_object<T>(end_);
                end_ += sizeof(T);
                return result;
            }

            /// \group construct
            T* value_construct_object()
            {
                auto result = array::value_construct_object<T>(end_);
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

        /// \effects Creates `n` objects of type `T` in the memory block using [array::default_construct_object]().
        /// \returns A pointer after the last created object.
        template <typename T>
        raw_pointer uninitialized_default_construct(const memory_block& block, size_type n)
        {
            partially_constructed_range<T> range(block);
            for (auto i = size_type(0); i != n; ++i)
                range.default_construct_object();
            return std::move(range).release();
        }

        /// \effects Creates `n` objects of type `T` in the memory block using [array::value_construct_object]().
        /// \returns A pointer after the last created object.
        template <typename T>
        raw_pointer uninitialized_value_construct(const memory_block& block, size_type n)
        {
            partially_constructed_range<T> range(block);
            for (auto i = size_type(0); i != n; ++i)
                range.value_construct_object();
            return std::move(range).release();
        }

        /// \effects Creates `n` objects of type `T` in the memory block by copying the existing one.
        /// \returns A pointer after the last created object.
        template <typename T>
        raw_pointer uninitialized_fill(const memory_block& block, size_type n, const T& obj)
        {
            partially_constructed_range<T> range(block);
            for (auto i = size_type(0); i != n; ++i)
                range.construct_object(obj);
            return std::move(range).release();
        }

        namespace detail
        {
            template <typename InputIter, typename T>
            struct can_memcpy
            : std::integral_constant<bool, is_contiguous_iterator<InputIter>::value
                                               && std::is_trivially_copyable<T>::value>
            {
            };

            template <typename T, typename, typename ContIter>
            raw_pointer uninitialized_move_copy_impl(std::true_type, ContIter begin, ContIter end,
                                                     const memory_block& block) noexcept
            {
                auto no_elements = std::size_t(end - begin);
                auto size        = no_elements * sizeof(T);
                std::memcpy(to_void_pointer(block.begin()), iterator_to_pointer(begin), size);
                return block.begin() + size;
            }

            template <typename T, typename TargetT, typename InputIter>
            raw_pointer uninitialized_move_copy_impl(std::false_type, InputIter begin,
                                                     InputIter end, const memory_block& block)
            {
                partially_constructed_range<T> range(block);
                for (auto cur = begin; cur != end; ++cur)
                    range.construct_object(static_cast<TargetT>(*cur));
                return std::move(range).release();
            }

            template <typename T, typename TargetT, typename InputIter>
            raw_pointer uninitialized_move_copy(InputIter begin, InputIter end,
                                                const memory_block& block)
            {
                return uninitialized_move_copy_impl<T, TargetT>(can_memcpy<InputIter, T>{}, begin,
                                                                end, block);
            }
        } // namespace detail

        /// \effects Moves elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, some objects in the original range may already be in the moved-from state,
        /// however all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_move(InputIter begin, InputIter end, const memory_block& block)
        {
            using type = typename std::iterator_traits<InputIter>::value_type;
            return detail::uninitialized_move_copy<type, type&&>(begin, end, block);
        }

        /// \effects [std::move_if_noexcept]() elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, the old range has not been modified and all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_move_if_noexcept(InputIter begin, InputIter end,
                                                   const memory_block& block)
        {
            using type = typename std::iterator_traits<InputIter>::value_type;
            using target_type =
                typename std::conditional<!std::is_nothrow_move_constructible<type>::value
                                              && std::is_copy_constructible<type>::value,
                                          const type&, type &&>::type;
            return detail::uninitialized_move_copy<type, target_type>(begin, end, block);
        }

        /// \effects Copies elements of the given range to the uninitialized memory of the given block.
        /// \returns A pointer past the last created object.
        /// \notes If an exception is thrown, the old range has not been modified and all objects created at the new location will be destroyed.
        template <typename InputIter>
        raw_pointer uninitialized_copy(InputIter begin, InputIter end, const memory_block& block)
        {
            using type = typename std::iterator_traits<InputIter>::value_type;
            return detail::uninitialized_move_copy<type, const type&>(begin, end, block);
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
