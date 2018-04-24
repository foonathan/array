// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/input_view.hpp>

#include <catch.hpp>

#include <foonathan/array/block_storage_new.hpp>

#include "leak_checker.hpp"

using namespace foonathan::array;

TEST_CASE("input_view", "[view]")
{
    using block_storage = block_storage_new<default_growth>;
    leak_checker checker;

    SECTION("steal")
    {
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}

            test_type(const test_type&) : leak_tracked()
            {
                FAIL("shouldn't have been called");
            }
        };

        block_storage storage({});
        auto          constructed = [&] {
            int ids[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};
            return assign_copy(storage, block_view<test_type>{}, std::begin(ids), std::end(ids));
        }();

        using view = input_view<test_type, block_storage>;
        view v(std::move(storage), constructed);
        REQUIRE(v.will_steal_memory());
        REQUIRE(!v.will_move());
        REQUIRE(!v.will_copy());

        REQUIRE(v.size() == 4u);

        block_storage new_storage({});
        auto          new_constructed = [&] {
            int ids[] = {0xE0E0, 0xE1E1};
            return assign_copy(new_storage, block_view<test_type>{}, std::begin(ids),
                               std::end(ids));
        }();

        new_constructed = std::move(v).release(new_storage, new_constructed);
        REQUIRE(new_constructed.size() == 4u);
        REQUIRE(new_constructed.data()[0].id == 0xF0F0);
        REQUIRE(new_constructed.data()[1].id == 0xF1F1);
        REQUIRE(new_constructed.data()[2].id == 0xF2F2);
        REQUIRE(new_constructed.data()[3].id == 0xF3F3);

        destroy_range(new_constructed.begin(), new_constructed.end());
    }
    SECTION("copy")
    {
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}

            test_type(const test_type&) = default;
            test_type& operator=(const test_type&) = default;
        };

        test_type block[] = {{0xF0F0}, {0xF1F1}, {0xF2F2}, {0xF3F3}};

        using view = input_view<test_type, block_storage>;
        view v(block);
        REQUIRE(!v.will_steal_memory());
        REQUIRE(!v.will_move());
        REQUIRE(v.will_copy());

        REQUIRE(v.size() == 4u);
        REQUIRE(v.view().data() == block);

        block_storage new_storage({});
        auto          new_constructed = [&] {
            int ids[] = {0xE0E0, 0xE1E1};
            return assign_copy(new_storage, block_view<test_type>{}, std::begin(ids),
                               std::end(ids));
        }();

        new_constructed = std::move(v).release(new_storage, new_constructed);
        REQUIRE(new_constructed.size() == 4u);
        REQUIRE(new_constructed.data()[0].id == 0xF0F0);
        REQUIRE(new_constructed.data()[1].id == 0xF1F1);
        REQUIRE(new_constructed.data()[2].id == 0xF2F2);
        REQUIRE(new_constructed.data()[3].id == 0xF3F3);

        destroy_range(new_constructed.begin(), new_constructed.end());
    }
    SECTION("move")
    {
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}

            test_type(test_type&&) = default;
            test_type& operator=(test_type&&) = default;
        };

        test_type block[] = {{0xF0F0}, {0xF1F1}, {0xF2F2}, {0xF3F3}};

        using view = input_view<test_type, block_storage>;
        view v(move(block));
        REQUIRE(!v.will_steal_memory());
        REQUIRE(v.will_move());
        REQUIRE(!v.will_copy());

        REQUIRE(v.size() == 4u);
        REQUIRE(v.mutable_view().data() == block);

        block_storage new_storage({});
        auto          new_constructed = [&] {
            int ids[] = {0xE0E0, 0xE1E1};
            return assign_copy(new_storage, block_view<test_type>{}, std::begin(ids),
                               std::end(ids));
        }();

        new_constructed = std::move(v).release(new_storage, new_constructed);
        REQUIRE(new_constructed.size() == 4u);
        REQUIRE(new_constructed.data()[0].id == 0xF0F0);
        REQUIRE(new_constructed.data()[1].id == 0xF1F1);
        REQUIRE(new_constructed.data()[2].id == 0xF2F2);
        REQUIRE(new_constructed.data()[3].id == 0xF3F3);

        destroy_range(new_constructed.begin(), new_constructed.end());
    }
}
