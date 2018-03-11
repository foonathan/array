// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/raw_storage.hpp>

#include <catch.hpp>
#include <cstdint>

#include "leak_checker.hpp"

using namespace foonathan::array;

TEST_CASE("construct_object/destroy_object/destroy_range", "[core]")
{
    leak_checker leak;

    struct test_type : leak_tracked
    {
        int sum;

        test_type(int a, int b) : sum(a + b) {}
    };

    std::aligned_storage<sizeof(test_type)>::type storage{};

    auto ptr = construct_object<test_type>(from_pointer(&storage), 40, 2);
    REQUIRE(ptr == static_cast<void*>(&storage));
    REQUIRE(ptr->sum == 42);

    SECTION("destroy_object")
    {
        auto raw_ptr = destroy_object(ptr);
        REQUIRE(to_void_pointer(raw_ptr) == static_cast<void*>(&storage));
    }
    SECTION("destroy_range")
    {
        destroy_range(ptr, ptr + 1);
    }
}

TEST_CASE("partially_constructed_range", "[core]")
{
    leak_checker leak;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}
    };
    std::aligned_storage<10 * sizeof(test_type)>::type storage{};

    partially_constructed_range<test_type> empty_range(from_pointer(&storage));

    auto first = empty_range.construct_object(0xF0F0);
    REQUIRE(first->id == 0xF0F0);

    auto second = empty_range.construct_object(0xF1F1);
    REQUIRE(second->id == 0xF1F1);

    auto third = empty_range.construct_object(0xF2F2);
    REQUIRE(third->id == 0xF2F2);

    SECTION("release")
    {
        auto end = std::move(empty_range).release();
        REQUIRE(end == from_pointer(&storage) + 3 * sizeof(test_type));
        destroy_range(first, third + 1);
    }
    SECTION("non-empty range")
    {
        auto end = std::move(empty_range).release();

        partially_constructed_range<test_type> non_empty(from_pointer(&storage), end);

        auto fourth = non_empty.construct_object(0xF3F3);
        REQUIRE(fourth->id == 0xF3F3);
    }
}

TEST_CASE("uninitialized_move", "[core]")
{
    leak_checker checker;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

        test_type(test_type&& other) : id(other.id)
        {
            if (id == 0xFFFF)
                throw "throwing move!";
        }

        test_type(const test_type&) = delete;
    };

    test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(from_pointer(&storage), 4 * sizeof(test_type));

    SECTION("nothrow")
    {
        auto end = uninitialized_move(std::begin(array), std::end(array), block);
        REQUIRE(end == from_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.memory);
        REQUIRE(ptr[0].id == 0xF0F0);
        REQUIRE(ptr[1].id == 0xF1F1);
        REQUIRE(ptr[2].id == 0xF2F2);
        REQUIRE(ptr[3].id == 0xF3F3);

        destroy_range(ptr, ptr + 4);
    }
    SECTION("throw")
    {
        array[2].id = 0xFFFF;
        REQUIRE_THROWS(uninitialized_move(std::begin(array), std::end(array), block));
    }
}

TEST_CASE("uninitialized_copy", "[core]")
{
    leak_checker checker;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

        test_type(const test_type& other) : id(other.id)
        {
            if (id == 0xFFFF)
                throw "throwing copy!";
        }
    };

    test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(from_pointer(&storage), 4 * sizeof(test_type));

    SECTION("nothrow")
    {
        auto end = uninitialized_copy(std::begin(array), std::end(array), block);
        REQUIRE(end == from_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.memory);
        REQUIRE(ptr[0].id == 0xF0F0);
        REQUIRE(ptr[1].id == 0xF1F1);
        REQUIRE(ptr[2].id == 0xF2F2);
        REQUIRE(ptr[3].id == 0xF3F3);

        destroy_range(ptr, ptr + 4);
    }
    SECTION("throw")
    {
        array[2].id = 0xFFFF;
        REQUIRE_THROWS(uninitialized_copy(std::begin(array), std::end(array), block));
    }
}

TEST_CASE("uninitialized_move_if_noexcept", "[core]")
{
    leak_checker checker;

    SECTION("noexcept move")
    {
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

            test_type(test_type&&) noexcept = default;

            test_type(const test_type&) : id(0)
            {
                FAIL("copy should not be used");
            }
        };

        test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

        std::aligned_storage<4 * sizeof(test_type)>::type storage{};
        auto block = memory_block(from_pointer(&storage), 4 * sizeof(test_type));

        auto end = uninitialized_move_if_noexcept(std::begin(array), std::end(array), block);
        REQUIRE(end == from_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.memory);
        REQUIRE(ptr[0].id == 0xF0F0);
        REQUIRE(ptr[1].id == 0xF1F1);
        REQUIRE(ptr[2].id == 0xF2F2);
        REQUIRE(ptr[3].id == 0xF3F3);

        destroy_range(ptr, ptr + 4);
    }
    SECTION("throwing move")
    {
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

            test_type(test_type&&) : id(0)
            {
                FAIL("move should not be used");
            }

            test_type(const test_type& other) : id(other.id)
            {
                if (id == 0xFFFF)
                    throw "throwing copy!";
            }
        };

        test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

        std::aligned_storage<4 * sizeof(test_type)>::type storage{};
        auto block = memory_block(from_pointer(&storage), 4 * sizeof(test_type));

        SECTION("nothrow")
        {
            auto end = uninitialized_move_if_noexcept(std::begin(array), std::end(array), block);
            REQUIRE(end == from_pointer(&storage) + 4 * sizeof(test_type));

            auto ptr = to_pointer<test_type>(block.memory);
            REQUIRE(ptr[0].id == 0xF0F0);
            REQUIRE(ptr[1].id == 0xF1F1);
            REQUIRE(ptr[2].id == 0xF2F2);
            REQUIRE(ptr[3].id == 0xF3F3);

            destroy_range(ptr, ptr + 4);
        }
        SECTION("throw")
        {
            array[2].id = 0xFFFF;
            REQUIRE_THROWS(
                uninitialized_move_if_noexcept(std::begin(array), std::end(array), block));
        }
    }
}

TEST_CASE("uninitialized_destructive_move", "[core]")
{
    leak_checker checker;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}
    };

    std::aligned_storage<8 * sizeof(test_type)>::type storage{};

    auto old_block = memory_block(from_pointer(&storage), 4 * sizeof(test_type));
    construct_object<test_type>(old_block.memory, 0xF0F0);
    construct_object<test_type>(old_block.memory + sizeof(test_type), 0xF1F1);
    construct_object<test_type>(old_block.memory + 2 * sizeof(test_type), 0xF2F2);
    construct_object<test_type>(old_block.memory + 3 * sizeof(test_type), 0xF3F3);

    auto new_block =
        memory_block(from_pointer(&storage) + 4 * sizeof(test_type), 4 * sizeof(test_type));

    // no need to check for throwing, this is already tested
    uninitialized_destructive_move(to_pointer<test_type>(old_block.memory),
                                   to_pointer<test_type>(old_block.memory + old_block.size),
                                   new_block);

    auto ptr = to_pointer<test_type>(new_block.memory);
    REQUIRE(ptr[0].id == 0xF0F0);
    REQUIRE(ptr[1].id == 0xF1F1);
    REQUIRE(ptr[2].id == 0xF2F2);
    REQUIRE(ptr[3].id == 0xF3F3);

    destroy_range(ptr, ptr + 4);
}
