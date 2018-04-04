// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/bag.hpp>

#include <catch.hpp>

#include <foonathan/array/block_storage_new.hpp>

#include "leak_checker.hpp"

using namespace foonathan::array;

namespace
{
    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
    };

    using test_bag = bag<test_type, block_storage_new<default_growth>>;

    void verify_bag_impl(const test_bag& bag, std::initializer_list<int> ids)
    {
        REQUIRE(bag.empty() == (bag.size() == 0u));
        REQUIRE(bag.size() == ids.end() - ids.begin());
        REQUIRE(bag.capacity() >= bag.size());
        REQUIRE(bag.capacity() <= bag.max_size());

        auto view = block_view<const test_type>(bag);
        REQUIRE(view.size() == bag.size());
        REQUIRE(view.data() == iterator_to_pointer(bag.begin()));
        REQUIRE(view.data_end() == iterator_to_pointer(bag.end()));
        REQUIRE(view.data() == iterator_to_pointer(bag.cbegin()));
        REQUIRE(view.data_end() == iterator_to_pointer(bag.cend()));

        if (!std::equal(view.begin(), view.end(), ids.begin(),
                        [](const test_type& test, int i) { return test.id == i; }))
        {
            for (auto el : view)
                FAIL_CHECK(std::hex << el.id);
            FAIL("not expected elements");
        }
    }

    void verify_bag(const test_bag& bag, std::initializer_list<int> ids)
    {
        verify_bag_impl(bag, ids);

        // copy constructor
        test_bag copy(bag);
        verify_bag_impl(copy, ids);
        REQUIRE(copy.capacity() <= bag.capacity());

        // shrink to fit
        auto old_cap = copy.capacity();
        copy.shrink_to_fit();
        verify_bag_impl(copy, ids);
        REQUIRE(copy.capacity() <= old_cap);

        // reserve
        copy.reserve(copy.size() + 4u);
        REQUIRE(copy.capacity() >= copy.size() + 4u);
        verify_bag_impl(copy, ids);

        // copy assignment
        copy.emplace(0xFFFF);
        copy = bag;
        verify_bag_impl(copy, ids);

        // range assignment
        copy.emplace(0xFFFF);
        copy.assign_range(bag.begin(), bag.end());
        verify_bag_impl(copy, ids);

        // block assignment
        copy.emplace(0xFFFF);
        copy.assign(bag);
        verify_bag_impl(copy, ids);
    }
} // namespace

TEST_CASE("bag", "[container]")
{
    leak_checker checker;

    SECTION("insertion")
    {
        test_bag bag;
        verify_bag(bag, {});

        bag.emplace(0xF0F0);
        verify_bag(bag, {0xF0F0});

        bag.insert(test_type(0xF1F1));
        verify_bag(bag, {0xF0F0, 0xF1F1});

        test_type test(0xF2F2);
        bag.insert(test);
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2});

        test_type tests[] = {{0xF3F3}, {0xF4F4}, {0xF5F5}};
        bag.insert_block(tests);
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});

        bag.insert_range(std::begin(tests), std::end(tests));
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4, 0xF5F5});

        SECTION("clear")
        {
            auto old_cap = bag.capacity();
            bag.clear();
            REQUIRE(old_cap == bag.capacity());
            verify_bag(bag, {});
        }
        SECTION("erase")
        {
            auto iter = bag.begin();
            iter      = bag.erase(iter);
            verify_bag(bag, {0xF5F5, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4});
            REQUIRE(iter == bag.begin());

            iter = bag.erase(std::next(iter, 3));
            verify_bag(bag, {0xF5F5, 0xF1F1, 0xF2F2, 0xF4F4, 0xF4F4, 0xF5F5, 0xF3F3});
            REQUIRE(iter == bag.begin() + 3);

            while (iter != bag.end())
                iter = bag.erase(iter);
            verify_bag(bag, {0xF5F5, 0xF1F1, 0xF2F2});
        }
        SECTION("erase range")
        {
            auto begin = bag.end() - 3;
            auto end   = bag.end();
            auto next  = bag.erase_range(begin, end);
            verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});
            REQUIRE(next == bag.end());

            begin = bag.begin() + 1;
            end   = bag.begin() + 3;
            next  = bag.erase_range(begin, end);
            verify_bag(bag, {0xF0F0, 0xF4F4, 0xF5F5, 0xF3F3});
            REQUIRE(next == bag.begin() + 1);

            begin = bag.begin() + 1;
            end   = bag.begin() + 3;
            next  = bag.erase_range(begin, end);
            verify_bag(bag, {0xF0F0, 0xF3F3});
            REQUIRE(next == bag.begin() + 1);
        }
        SECTION("move constructor")
        {
            auto     data = iterator_to_pointer(bag.begin());
            test_bag other(std::move(bag));
            verify_bag(other,
                       {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4, 0xF5F5});
            verify_bag(bag, {});
            REQUIRE(iterator_to_pointer(other.begin()) == data);

            SECTION("copy assignment")
            {
                bag = other;
                verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                 0xF5F5});
                verify_bag(other, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                   0xF5F5});
            }
            SECTION("move assignment")
            {
                bag = std::move(other);
                verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                 0xF5F5});
                verify_bag(other, {});
                REQUIRE(iterator_to_pointer(bag.begin()) == data);
            }
            SECTION("input_view assignment")
            {
                bag = input_view<test_type, block_storage_new<default_growth>>(std::move(other));
                verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                 0xF5F5});
                verify_bag(other, {});
                REQUIRE(iterator_to_pointer(bag.begin()) == data);
            }
            SECTION("swap")
            {
                swap(bag, other);
                verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                 0xF5F5});
                verify_bag(other, {});
                REQUIRE(iterator_to_pointer(bag.begin()) == data);
            }
        }
    }
    SECTION("input_view ctor")
    {
        test_bag bag{{test_type(0xF0F0), test_type(0xF1F1), test_type(0xF2F2), test_type(0xF3F3)}};
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});
    }
}
