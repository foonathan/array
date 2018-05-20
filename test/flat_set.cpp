// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/flat_set.hpp>

#include <catch.hpp>

#include "equal_checker.hpp"
#include "leak_checker.hpp"

using namespace foonathan::array;

namespace
{
    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int i) : id(static_cast<std::uint16_t>(i)) {}

        int compare(const test_type& other) const
        {
            return compare(other.id);
        }

        int compare(int other) const
        {
            if (id == other)
                return 0;
            else if (id < other)
                return -1;
            else
                return +1;
        }
    };

    using test_set      = flat_set<test_type>;
    using test_multiset = flat_multiset<test_type>;

    template <class Set>
    void verify_set_impl(const Set& set, std::initializer_list<int> ids)
    {
        REQUIRE(set.empty() == (set.size() == 0u));
        REQUIRE(set.size() == size_type(ids.end() - ids.begin()));
        REQUIRE(set.capacity() >= set.size());
        REQUIRE(set.capacity() <= set.max_size());

        auto view = block_view<const test_type>(set);
        REQUIRE(view.size() == set.size());
        REQUIRE(view.data() == iterator_to_pointer(set.begin()));
        REQUIRE(view.data_end() == iterator_to_pointer(set.end()));
        REQUIRE(view.data() == iterator_to_pointer(set.cbegin()));
        REQUIRE(view.data_end() == iterator_to_pointer(set.cend()));
        check_equal(view.begin(), view.end(), ids.begin(), ids.end(),
                    [](const test_type& test, int i) { return test.id == i; },
                    [&](const test_type& test) { FAIL_CHECK(std::hex << test.id); });

        if (!set.empty())
        {
            REQUIRE(set.min().id == *ids.begin());
            REQUIRE(set.max().id == *std::prev(ids.end()));
        }

        auto cur_index = size_type(0);
        auto last_id   = -1;
        for (auto& id : ids)
        {
            if (last_id != -1 && last_id != id)
                cur_index += set.count(last_id);

            REQUIRE(set.contains(id));

            REQUIRE(size_type(set.find(id) - set.begin()) == cur_index);

            REQUIRE(size_type(set.lower_bound(id) - set.begin()) == cur_index);
            REQUIRE(size_type(set.upper_bound(id) - set.begin()) == cur_index + set.count(id));

            auto range = set.equal_range(id);
            REQUIRE(size_type(range.begin() - set.begin()) == cur_index);
            REQUIRE(std::next(range.begin(), std::ptrdiff_t(set.count(id))) == range.end());

            last_id = id;
        }
    }

    template <class Set>
    void verify_set(const Set& set, std::initializer_list<int> ids)
    {
        verify_set_impl(set, ids);

        // copy constructor
        Set copy(set);
        verify_set_impl(copy, ids);
        REQUIRE(copy.capacity() <= set.capacity());

        // shrink to fit
        auto old_cap = copy.capacity();
        copy.shrink_to_fit();
        verify_set_impl(copy, ids);
        REQUIRE(copy.capacity() <= old_cap);

        // reserve
        copy.reserve(copy.size() + 4u);
        REQUIRE(copy.capacity() >= copy.size() + 4u);
        verify_set_impl(copy, ids);

        // copy assignment
        copy.insert(0xFFFF);
        copy = set;
        verify_set_impl(copy, ids);

        // range assignment
        copy.insert(0xFFFF);
        copy.assign_range(set.begin(), set.end());
        verify_set_impl(copy, ids);

        // block assignment
        copy.insert(0xFFFF);
        copy.assign(set);
        verify_set_impl(copy, ids);
    }

    void verify_result(const test_set& set, test_set::insert_result result, int id, std::size_t pos,
                       bool duplicate = false)
    {
        if (duplicate)
        {
            REQUIRE(!result.was_inserted());
            REQUIRE(result.was_duplicate());
        }
        else
        {
            REQUIRE(result.was_inserted());
            REQUIRE(!result.was_duplicate());
        }

        REQUIRE(result.iter()->id == id);
        REQUIRE(std::size_t(result.iter() - set.begin()) == pos);
    }
    void verify_result(const test_multiset& set, test_multiset::insert_result result, int id,
                       std::size_t pos, bool duplicate = false)
    {
        REQUIRE(result.was_inserted());
        REQUIRE(result.was_duplicate() == duplicate);

        REQUIRE(result.iter()->id == id);
        REQUIRE(std::size_t(result.iter() - set.begin()) == pos);
    }
} // namespace

TEST_CASE("flat_set", "[container]")
{
    leak_checker checker;

    SECTION("insertion")
    {
        test_set set;
        verify_set(set, {});

        // fill with non-duplicates
        auto result = set.try_emplace(0xF0F0);
        verify_result(set, result, 0xF0F0, 0);
        verify_set(set, {0xF0F0});

        result = set.insert(test_type(0xF3F3));
        verify_result(set, result, 0xF3F3, 1);
        verify_set(set, {0xF0F0, 0xF3F3});

        test_type test(0xF2F2);
        result = set.insert(test);
        verify_result(set, result, 0xF2F2, 1);
        verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});

        SECTION("range insert")
        {
            test_type tests[] = {0xF4F4, 0xF5F5};
            set.insert(tests);
            verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});

            set.insert_range(std::begin(tests), std::end(tests));
            verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});
        }
        SECTION("duplicate insert")
        {
            result = set.insert(0xF0F0);
            verify_result(set, result, 0xF0F0, 0, true);

            result = set.insert(0xF3F3);
            verify_result(set, result, 0xF3F3, 2, true);
        }
        SECTION("clear")
        {
            auto old_cap = set.capacity();
            set.clear();
            REQUIRE(old_cap == set.capacity());
            verify_set(set, {});
        }
        SECTION("erase")
        {
            set.insert(0xF1F1);
            verify_set(set, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});

            auto iter = set.erase(set.begin());
            verify_set(set, {0xF1F1, 0xF2F2, 0xF3F3});
            REQUIRE(iter == set.begin());

            iter = set.erase(std::next(set.begin()));
            verify_set(set, {0xF1F1, 0xF3F3});
            REQUIRE(iter == std::prev(set.end()));

            iter = set.erase(std::prev(set.end()));
            verify_set(set, {0xF1F1});
            REQUIRE(iter == set.end());
        }
        SECTION("erase_range")
        {
            set.insert(0xF1F1);
            verify_set(set, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});

            auto iter = set.erase_range(std::next(set.begin()), std::prev(set.end(), 2));
            verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});
            REQUIRE(iter == std::next(set.begin()));

            iter = set.erase_range(std::next(set.begin()), set.end());
            verify_set(set, {0xF0F0});
            REQUIRE(iter == set.end());

            iter = set.erase_range(set.begin(), set.begin());
            verify_set(set, {0xF0F0});
            REQUIRE(iter == set.begin());
        }
        SECTION("erase_all")
        {
            set.insert(0xF1F1);
            verify_set(set, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});

            auto erased = set.erase_all(0xF0F0);
            verify_set(set, {0xF1F1, 0xF2F2, 0xF3F3});
            REQUIRE(erased);

            erased = set.erase_all(0xF4F4);
            verify_set(set, {0xF1F1, 0xF2F2, 0xF3F3});
            REQUIRE(!erased);
        }
        SECTION("lookup")
        {
            // lookup of existing items already checked in verify_set()

            REQUIRE(!set.contains(0xF4F4));
            REQUIRE(set.find(0xF4F4) == set.end());
            REQUIRE(set.lower_bound(0xF4F4) == set.end());
            REQUIRE(set.upper_bound(0xF4F4) == set.end());

            auto range = set.equal_range(0xF4F4);
            REQUIRE(range.empty());
            REQUIRE(range.begin() == set.end());
            REQUIRE(range.end() == set.end());
        }
        SECTION("move constructor")
        {
            auto     data = iterator_to_pointer(set.begin());
            test_set other(std::move(set));
            verify_set(other, {0xF0F0, 0xF2F2, 0xF3F3});
            verify_set(set, {});
            REQUIRE(iterator_to_pointer(other.begin()) == data);

            SECTION("copy assignment")
            {
                set = other;
                verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});
                verify_set(other, {0xF0F0, 0xF2F2, 0xF3F3});
            }
            SECTION("move assignment")
            {
                set = std::move(other);
                verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});
                verify_set(other, {});
                REQUIRE(iterator_to_pointer(set.begin()) == data);
            }
            SECTION("input_view assignment")
            {
                set = input_view<test_type, block_storage_new<default_growth>>(std::move(other));
                verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});
                REQUIRE(iterator_to_pointer(set.begin()) == data);
            }
            SECTION("range assignment")
            {
                int ids[] = {0xF0F0, 0xF2F2, 0xF1F1};
                set.assign_range(std::begin(ids), std::end(ids));
                verify_set(set, {0xF0F0, 0xF1F1, 0xF2F2});
            }
            SECTION("swap")
            {
                swap(set, other);
                verify_set(set, {0xF0F0, 0xF2F2, 0xF3F3});
                verify_set(other, {});
                REQUIRE(iterator_to_pointer(set.begin()) == data);
            }
        }
    }
    SECTION("input_view ctor")
    {
        test_set set{{test_type(0xF0F0), test_type(0xF1F1), test_type(0xF2F2), test_type(0xF3F3),
                      test_type(0xF0F0)}};
        verify_set(set, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});
    }
}

TEST_CASE("flat_multiset", "[container]")
{
    // only check duplicate stuff

    leak_checker checker;

    test_multiset set{{test_type(0xF0F0), test_type(0xF1F1), test_type(0xF2F2), test_type(0xF0F0)}};
    verify_set(set, {0xF0F0, 0xF0F0, 0xF1F1, 0xF2F2});

    auto result = set.insert(0xF3F3);
    verify_set(set, {0xF0F0, 0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});
    verify_result(set, result, 0xF3F3, 4);

    result = set.insert(0xF1F1);
    verify_set(set, {0xF0F0, 0xF0F0, 0xF1F1, 0xF1F1, 0xF2F2, 0xF3F3});
    verify_result(set, result, 0xF1F1, 3, true);
}
