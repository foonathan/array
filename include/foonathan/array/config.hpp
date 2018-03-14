// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED
#define FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED

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

#endif // FOONATHAN_ARRAY_CONFIG_HPP_INCLUDED
