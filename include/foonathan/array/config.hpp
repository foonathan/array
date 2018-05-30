// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED
#define FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED

#include <cstddef>
#include <type_traits>

#ifndef FOONATHAN_ARRAY_USE_CONSTEXPR14

#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304
/// \exclude
#define FOONATHAN_ARRAY_USE_CONSTEXPR14 1
#else
/// \exclude
#define FOONATHAN_ARRAY_USE_CONSTEXPR14 0
#endif

#endif // FOONATHAN_ARRAY_USE_CONSTEXPR14

#if FOONATHAN_ARRAY_USE_CONSTEXPR14
/// \exclude
#define FOONATHAN_ARRAY_CONSTEXPR14 constexpr
#else
/// \exclude
#define FOONATHAN_ARRAY_CONSTEXPR14
#endif

#ifndef FOONATHAN_ARRAY_HAS_BYTE

#if defined(__cpp_lib_byte)
/// \exclude
#define FOONATHAN_ARRAY_HAS_BYTE 1
#else
/// \exclude
#define FOONATHAN_ARRAY_HAS_BYTE 0
#endif

#endif // FOONATHAN_ARRAY_HAS_BYTE

#ifndef FOONATHAN_ARRAY_HAS_SWAPPABLE

#if defined(__cpp_lib_is_swappable)
/// \exclude
#define FOONATHAN_ARRAY_HAS_SWAPPABLE 1
#else
/// \exclude
#define FOONATHAN_ARRAY_HAS_SWAPPABLE 0
#endif

#endif // FOONATHAN_ARRAY_HAS_SWAPPABLE

#endif // FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED
