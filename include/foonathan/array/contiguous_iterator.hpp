// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_CONTIGUOUS_ITERATOR_HPP_INCLUDED
#define FOONATHAN_ARRAY_CONTIGUOUS_ITERATOR_HPP_INCLUDED

#include <type_traits>

namespace foonathan
{
    namespace array
    {
        /// Type trait to check whether a given iterator is a C++17 `ContiguousIterator`,
        /// i.e. whether it can be converted to a pointer.
        ///
        /// Custom iterators may specialize this trait in order to mark themselves as contiguous.
        ///
        /// All iterators in this library are contiguous.
        template <typename T>
        struct is_contiguous_iterator : std::false_type
        {
            /// \returns A pointer pointing to the same location as the iterator.
            /// This function must be implemented by a specialization.
            /// \notes The return type should not be `void*`, but properly typed.
            static void* to_pointer(const T& iterator) noexcept = delete;
        };

        /// Specialization to mark plain pointers as contiguous iterators.
        template <typename T>
        struct is_contiguous_iterator<T*> : std::true_type
        {
            /// \returns The pointer unchanged.
            static T* to_pointer(T* iterator) noexcept
            {
                return iterator;
            }
        };

        /// \returns A pointer pointing to the same location as the contiguous iterator.
        /// \notes This function only participates in overload resolution if it actually is a contiguous iterator.
        template <typename ContIter>
        auto iterator_to_pointer(const ContIter& iter) noexcept
            -> decltype(is_contiguous_iterator<ContIter>::to_pointer(iter))
        {
            return is_contiguous_iterator<ContIter>::to_pointer(iter);
        }
    }
} // namespace foonathan::array

#endif // FOONATHAN_ARRAY_CONTIGUOUS_ITERATOR_HPP_INCLUDED
