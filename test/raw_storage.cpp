// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/raw_storage.hpp>

#include <catch.hpp>
#include <cstdint>
#include <initializer_list>

#include "leak_checker.hpp"

using namespace foonathan::array;

TEST_CASE("*_construct_object", "[core]")
{
    struct test_type
    {
        int sum;

        test_type(int a, int b) : sum(a + b) {}
        test_type(std::initializer_list<int> list) : sum(*list.begin()) {}
    };

    std::aligned_storage<sizeof(int)>::type storage{};

    // test using partially_constructed_range to test member functions as well
    SECTION("default")
    {
        partially_constructed_range<int> range(as_raw_pointer(&storage));
        range.default_construct_object();
        // can't really check the result here...
    }
    SECTION("value")
    {
        partially_constructed_range<int> range(as_raw_pointer(&storage));
        auto                             ptr = range.default_construct_object();
        REQUIRE(*ptr == 0);
    }
    SECTION("paren")
    {
        partially_constructed_range<test_type> range(as_raw_pointer(&storage));
        auto                                   ptr = range.paren_construct_object(1, 2);
        REQUIRE(ptr->sum == 3);
    }
    SECTION("brace")
    {
        partially_constructed_range<test_type> range(as_raw_pointer(&storage));
        auto                                   ptr = range.brace_construct_object(1, 2, 3);
        REQUIRE(ptr->sum == 1);
    }
    SECTION("paren or brace")
    {
        struct aggregate
        {
            int i;
        };

        partially_constructed_range<aggregate> range(as_raw_pointer(&storage));
        auto                                   ptr = range.construct_object(1);
        REQUIRE(ptr->i == 1);
    }
}

TEST_CASE("construct_object/destroy_object/destroy_range", "[core]")
{
    leak_checker leak;

    struct test_type : leak_tracked
    {
        int sum;

        test_type(int a, int b) : sum(a + b) {}
    };

    std::aligned_storage<sizeof(test_type)>::type storage{};

    auto ptr = construct_object<test_type>(as_raw_pointer(&storage), 40, 2);
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

    partially_constructed_range<test_type> empty_range(as_raw_pointer(&storage));

    auto first = empty_range.construct_object(0xF0F0);
    REQUIRE(first->id == 0xF0F0);

    auto second = empty_range.construct_object(0xF1F1);
    REQUIRE(second->id == 0xF1F1);

    auto third = empty_range.construct_object(0xF2F2);
    REQUIRE(third->id == 0xF2F2);

    SECTION("release")
    {
        auto end = std::move(empty_range).release();
        REQUIRE(end == as_raw_pointer(&storage) + 3 * sizeof(test_type));
        destroy_range(first, third + 1);
    }
    SECTION("non-empty range")
    {
        auto end = std::move(empty_range).release();

        partially_constructed_range<test_type> non_empty(as_raw_pointer(&storage), end);

        auto fourth = non_empty.construct_object(0xF3F3);
        REQUIRE(fourth->id == 0xF3F3);
    }
}

TEST_CASE("uninitialized_ construction", "[core]")
{
    leak_checker leak;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type() = default;
        test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
    };
    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

    raw_pointer end;
    SECTION("default construct")
    {
        end = uninitialized_default_construct<test_type>(block, 4);
        // can't check the values
    }
    SECTION("value construct")
    {
        end = uninitialized_value_construct<test_type>(block, 4);

        auto ptr = to_pointer<test_type>(block.begin());
        REQUIRE(ptr[0].id == 0);
        REQUIRE(ptr[1].id == 0);
        REQUIRE(ptr[2].id == 0);
        REQUIRE(ptr[3].id == 0);
    }
    SECTION("fill")
    {
        end = uninitialized_fill(block, 4, test_type(0xF0F0));

        auto ptr = to_pointer<test_type>(block.begin());
        REQUIRE(ptr[0].id == 0xF0F0);
        REQUIRE(ptr[1].id == 0xF0F0);
        REQUIRE(ptr[2].id == 0xF0F0);
        REQUIRE(ptr[3].id == 0xF0F0);
    }

    REQUIRE(end == block.end());
    destroy_range(to_pointer<test_type>(block.begin()), to_pointer<test_type>(end));
}

TEST_CASE("uninitialized_move/move_if_noexcept/copy for trivial type", "[core]")
{
    struct test_type
    {
        std::uint16_t id;

        test_type(int i) : id(std::uint16_t(i)) {}

        // this type is only moveable, and not copyable
        // so if the memcpy optimization didn't kick in,
        // uninitialized_copy would not compile
        test_type(const test_type&) = delete;
        test_type(test_type&&)      = default;
        ~test_type()                = default;
    };
    REQUIRE(std::is_trivially_copyable<test_type>::value);

    test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

    raw_pointer end;
    SECTION("uninitialized_move")
    {
        end = uninitialized_move(std::begin(array), std::end(array), block);
    }
    SECTION("uninitialized_move_if_noexcept")
    {
        end = uninitialized_move_if_noexcept(std::begin(array), std::end(array), block);
    }
    SECTION("uninitialized_copy")
    {
        end = uninitialized_copy(std::begin(array), std::end(array), block);
    }
    REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

    auto ptr = to_pointer<test_type>(block.begin());
    REQUIRE(ptr[0].id == 0xF0F0);
    REQUIRE(ptr[1].id == 0xF1F1);
    REQUIRE(ptr[2].id == 0xF2F2);
    REQUIRE(ptr[3].id == 0xF3F3);
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
    auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

    SECTION("nothrow")
    {
        auto end = uninitialized_move(std::begin(array), std::end(array), block);
        REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.begin());
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

            test_type(const test_type& other) : leak_tracked(other), id(0)
            {
                FAIL("copy should not be used");
            }
        };

        test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

        std::aligned_storage<4 * sizeof(test_type)>::type storage{};
        auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

        auto end = uninitialized_move_if_noexcept(std::begin(array), std::end(array), block);
        REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.begin());
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

            test_type(const test_type& other) : leak_tracked(other), id(other.id)
            {
                if (id == 0xFFFF)
                    throw "throwing copy!";
            }
        };

        test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

        std::aligned_storage<4 * sizeof(test_type)>::type storage{};
        auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

        SECTION("nothrow")
        {
            auto end = uninitialized_move_if_noexcept(std::begin(array), std::end(array), block);
            REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

            auto ptr = to_pointer<test_type>(block.begin());
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

TEST_CASE("uninitialized_copy", "[core]")
{
    leak_checker checker;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

        test_type(const test_type& other) : leak_tracked(other), id(other.id)
        {
            if (id == 0xFFFF)
                throw "throwing copy!";
        }
    };

    test_type array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

    SECTION("nothrow")
    {
        auto end = uninitialized_copy(std::begin(array), std::end(array), block);
        REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.begin());
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

TEST_CASE("uninitialized_copy_convert", "[core]")
{
    leak_checker checker;

    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int id) : id(static_cast<std::uint16_t>(id)) {}

        test_type(const test_type& other) : leak_tracked(other), id(other.id)
        {
            if (id == 0xFFFF)
                throw "throwing copy!";
        }
    };

    int array[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3};

    std::aligned_storage<4 * sizeof(test_type)>::type storage{};
    auto block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));

    SECTION("nothrow")
    {
        auto end = uninitialized_copy_convert<test_type>(std::begin(array), std::end(array), block);
        REQUIRE(end == as_raw_pointer(&storage) + 4 * sizeof(test_type));

        auto ptr = to_pointer<test_type>(block.begin());
        REQUIRE(ptr[0].id == 0xF0F0);
        REQUIRE(ptr[1].id == 0xF1F1);
        REQUIRE(ptr[2].id == 0xF2F2);
        REQUIRE(ptr[3].id == 0xF3F3);

        destroy_range(ptr, ptr + 4);
    }
    SECTION("throw")
    {
        array[2] = 0xFFFF;
        REQUIRE_THROWS(
            uninitialized_copy_convert<test_type>(std::begin(array), std::end(array), block));
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

    auto old_block = memory_block(as_raw_pointer(&storage), 4 * sizeof(test_type));
    paren_construct_object<test_type>(old_block.begin(), 0xF0F0);
    paren_construct_object<test_type>(old_block.begin() + sizeof(test_type), 0xF1F1);
    paren_construct_object<test_type>(old_block.begin() + 2 * sizeof(test_type), 0xF2F2);
    paren_construct_object<test_type>(old_block.begin() + 3 * sizeof(test_type), 0xF3F3);

    auto new_block =
        memory_block(as_raw_pointer(&storage) + 4 * sizeof(test_type), 4 * sizeof(test_type));

    // no need to check for throwing, this is already tested
    uninitialized_destructive_move(to_pointer<test_type>(old_block.begin()),
                                   to_pointer<test_type>(old_block.end()), new_block);

    auto ptr = to_pointer<test_type>(new_block.begin());
    REQUIRE(ptr[0].id == 0xF0F0);
    REQUIRE(ptr[1].id == 0xF1F1);
    REQUIRE(ptr[2].id == 0xF2F2);
    REQUIRE(ptr[3].id == 0xF3F3);

    destroy_range(ptr, ptr + 4);
}
