// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_DETAIL_IS_TRIVIAL_HPP_INCLUDED
#define FOONATHAN_ARRAY_DETAIL_IS_TRIVIAL_HPP_INCLUDED

#include <foonathan/array/config.hpp>

namespace foonathan
{
    namespace array
    {
        namespace detail
        {
#if defined(__GLIBCXX__) && !defined(_GLIBCXX_RELEASE) && __GNUC__ < 5
            // if _GLIBCXX_RELEASE is defined we're in libstdc++ 7 or greater
            // if it isn't defined we check using the GCC version which is incorrect for clang...

#if !defined(__clang__) && defined(__GNUC_MINOR__) && __GNUC_MINOR__ <= 8
// types with deleted copy ctor are not considered trivially copyable in GCC 4.8
#define FOONATHAN_ARRAY_IS_DELETED_TRIVIALLY_COPYABLE false
#else
#define FOONATHAN_ARRAY_IS_DELETED_TRIVIALLY_COPYABLE true
#endif

            // no std::is_trivially_copyable yet
            template <typename T>
            using is_trivially_copyable = std::integral_constant<bool, __has_trivial_copy(T)>;
#else

#if defined(_MSC_VER)
// under MSVC types with deleted copy ctor are not considered trivially copyable (at all?)
#define FOONATHAN_ARRAY_IS_DELETED_TRIVIALLY_COPYABLE false
#else
#define FOONATHAN_ARRAY_IS_DELETED_TRIVIALLY_COPYABLE true
#endif

            template <typename T>
            using is_trivially_copyable = std::is_trivially_copyable<T>;
#endif

        } // namespace detail
    }     // namespace array
} // namespace foonathan

#endif // FOONATHAN_ARRAY_DETAIL_IS_TRIVIAL_HPP_INCLUDED
