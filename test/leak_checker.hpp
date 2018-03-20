// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_LEAK_CHECKER_HPP_INCLUDED
#define FOONATHAN_ARRAY_LEAK_CHECKER_HPP_INCLUDED

#include <catch.hpp>

namespace
{
    thread_local unsigned leak_count = 0u;

    struct leak_checker
    {
        unsigned old;

        leak_checker() : old(leak_count) {}

        leak_checker(const leak_checker&) = delete;
        leak_checker& operator=(const leak_checker&) = delete;

        ~leak_checker() noexcept(false)
        {
            if (leak_count != old)
                FAIL("leaking objects somewhere");
        }
    };

    struct leak_tracked
    {
    protected:
        leak_tracked() noexcept
        {
            ++leak_count;
        }

        leak_tracked(const leak_tracked&) noexcept
        {
            ++leak_count;
        }

        ~leak_tracked() noexcept
        {
            --leak_count;
        }

        leak_tracked& operator=(const leak_tracked&) noexcept = default;
    };
}

#endif // FOONATHAN_ARRAY_LEAK_CHECKER_HPP_INCLUDED
