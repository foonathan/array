// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/flat_map.hpp>

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

    using test_map = flat_map<test_type, std::string>;

    template <class Map>
    void verify_map_impl(Map& map, std::initializer_list<int> ids,
                         std::initializer_list<std::string> strs)
    {
        REQUIRE(map.empty() == (map.size() == 0u));
        REQUIRE(map.size() == size_type(ids.end() - ids.begin()));
        REQUIRE(map.capacity() >= map.size());
        REQUIRE(map.capacity() <= map.max_size());

        auto keys = map.keys();
        REQUIRE(keys.size() == map.size());
        REQUIRE(keys.data() == iterator_to_pointer(map.key_begin()));
        REQUIRE(keys.data_end() == iterator_to_pointer(map.key_end()));
        REQUIRE(keys.data() == iterator_to_pointer(map.key_cbegin()));
        REQUIRE(keys.data_end() == iterator_to_pointer(map.key_cend()));
        check_equal(keys.begin(), keys.end(), ids.begin(), ids.end(),
                    [](const test_type& test, int i) { return test.id == i; },
                    [&](const test_type& test) { FAIL_CHECK(std::hex << test.id); });

        auto values = map.values();
        REQUIRE(values.size() == map.size());
        REQUIRE(values.data() == iterator_to_pointer(map.value_begin()));
        REQUIRE(values.data_end() == iterator_to_pointer(map.value_end()));
        REQUIRE(values.data() == iterator_to_pointer(map.value_cbegin()));
        REQUIRE(values.data_end() == iterator_to_pointer(map.value_cend()));
        check_equal(values.begin(), values.end(), strs.begin(), strs.end(),
                    [](const std::string& test, const std::string& str) { return test == str; },
                    [&](const std::string& test) { FAIL_CHECK(test); });

        auto cur_id  = ids.begin();
        auto cur_str = strs.begin();
        for (auto pair : map)
        {
            REQUIRE(pair.key.id == *cur_id);
            REQUIRE(pair.value == *cur_str);
            ++cur_id;
            ++cur_str;
        }

        REQUIRE(map.value_end() == map.value_iter(map.end()));
        REQUIRE(map.value_end() == map.value_iter(map.key_end()));
        REQUIRE(map.key_end() == map.key_iter(map.end()));
        REQUIRE(map.key_end() == map.key_iter(map.value_end()));
        REQUIRE(map.end() == map.key_value_iter(map.key_end()));
        REQUIRE(map.end() == map.key_value_iter(map.value_end()));

        if (!map.empty())
        {
            REQUIRE(map.min().key.id == *ids.begin());
            REQUIRE(map.max().key.id == *std::prev(ids.end()));
        }

        auto cur_index = size_type(0);
        auto last_id   = -1;
        cur_str        = strs.begin();
        for (auto& id : ids)
        {
            if (last_id != -1 && last_id != id)
                cur_index += map.count(last_id);

            REQUIRE(map.contains(id));

            auto ptr = map.try_lookup(id);
            REQUIRE(ptr);
            REQUIRE(*ptr == *cur_str);
            REQUIRE(&map.lookup(id) == ptr);
            REQUIRE(size_type(ptr - iterator_to_pointer(map.value_begin())) == cur_index);

            REQUIRE(size_type(map.find(id) - map.begin()) == cur_index);

            REQUIRE(size_type(map.lower_bound(id) - map.begin()) == cur_index);
            REQUIRE(size_type(map.upper_bound(id) - map.begin()) == cur_index + map.count(id));

            auto range = map.equal_range(id);
            REQUIRE(size_type(range.begin() - map.begin()) == cur_index);
            REQUIRE(std::next(range.begin(), std::ptrdiff_t(map.count(id))) == range.end());

            last_id = id;
            ++cur_str;
        }
    }

    template <class Map>
    void verify_map(const Map& map, std::initializer_list<int> ids,
                    std::initializer_list<std::string> strs)
    {
        verify_map_impl(map, ids, strs);
        verify_map_impl(const_cast<Map&>(map), ids, strs); // to test non-const overloads

        // copy constructor
        Map copy(map);
        verify_map_impl(copy, ids, strs);
        REQUIRE(copy.capacity() <= map.capacity());

        // shrink to fit
        auto old_cap = copy.capacity();
        copy.shrink_to_fit();
        verify_map_impl(copy, ids, strs);
        REQUIRE(copy.capacity() <= old_cap);

        // reserve
        copy.reserve(copy.size() + 4u);
        REQUIRE(copy.capacity() >= copy.size() + 4u);
        verify_map_impl(copy, ids, strs);

        // copy assignment
        copy.insert(0xFFFF, "");
        copy = map;
        verify_map_impl(copy, ids, strs);

        // range assignment
        copy.insert(0xFFFF, "");
        copy.assign_range(map.key_begin(), map.key_end(), map.value_begin(), map.value_end());
        verify_map_impl(copy, ids, strs);

        // pair range assignment
        copy.insert(0xFFFF, "");
        copy.assign_pair_range(map.begin(), map.end());
        verify_map_impl(copy, ids, strs);
    }

    void verify_result(const test_map& map, test_map::insert_result result, int id, std::string str,
                       std::size_t pos, bool duplicate = false)
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

        REQUIRE(result.iter()->key.id == id);
        REQUIRE(result.iter()->value == str);
        REQUIRE(std::size_t(result.iter() - map.begin()) == pos);
    }
} // namespace

TEST_CASE("flat_map", "[container]")
{
    leak_checker checker;

    SECTION("insertion")
    {
        test_map map;
        verify_map(map, {}, {});

        // fill with non duplicates
        auto result = map.try_emplace(0xF0F0, "a");
        verify_result(map, result, 0xF0F0, "a", 0);
        verify_map(map, {0xF0F0}, {"a"});

        result = map.insert(test_type(0xF3F3), "d");
        verify_result(map, result, 0xF3F3, "d", 1);
        verify_map(map, {0xF0F0, 0xF3F3}, {"a", "d"});

        result = map.insert(0xF1F1, "b");
        verify_result(map, result, 0xF1F1, "b", 1);
        verify_map(map, {0xF0F0, 0xF1F1, 0xF3F3}, {"a", "b", "d"});

        result = map.insert_pair(std::make_pair(test_type(0xF2F2), "c"));
        verify_result(map, result, 0xF2F2, "c", 2);
        verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});

        SECTION("range insert")
        {
            test_type   keys[]   = {test_type(0xF4F4), test_type(0xF6F6), test_type(0xF5F5),
                                test_type(0xF1F1)};
            std::string values[] = {"e", "g", "f", "x"};
            map.insert_range(std::begin(keys), std::end(keys), std::begin(values),
                             std::end(values));
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF6F6},
                       {"a", "b", "c", "d", "e", "f", "g"});

            std::pair<int, std::string> pairs[] = {{0xF8F8, "i"}, {0xF7F7, "h"}};
            map.insert_pair_range(std::begin(pairs), std::end(pairs));
            verify_map(map,
                       {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF6F6, 0xF7F7, 0xF8F8},
                       {"a", "b", "c", "d", "e", "f", "g", "h", "i"});
        }
        SECTION("duplicate insert")
        {
            result = map.insert(0xF1F1, "x");
            verify_result(map, result, 0xF1F1, "b", 1, true);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});

            result = map.insert(0xF3F3, "x");
            verify_result(map, result, 0xF3F3, "d", 3, true);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
        }
        SECTION("assign insert")
        {
            result = map.emplace_or_assign(0xF1F1, 2, 'b');
            verify_result(map, result, 0xF1F1, "bb", 1, true);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "bb", "c", "d"});

            result = map.insert_or_assign(0xF4F4, "e");
            verify_result(map, result, 0xF4F4, "e", 4);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4}, {"a", "bb", "c", "d", "e"});

            result = map.insert_or_assign_pair(std::make_pair(0xF2F2, "cc"));
            verify_result(map, result, 0xF2F2, "cc", 2, true);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4}, {"a", "bb", "cc", "d", "e"});
        }
        SECTION("clear")
        {
            auto old_cap = map.capacity();
            map.clear();
            REQUIRE(old_cap == map.capacity());
            verify_map(map, {}, {});
        }
        SECTION("erase")
        {
            auto iter = map.erase(map.begin());
            verify_map(map, {0xF1F1, 0xF2F2, 0xF3F3}, {"b", "c", "d"});
            REQUIRE(iter == map.begin());

            iter = map.erase(std::prev(map.end(), 2));
            verify_map(map, {0xF1F1, 0xF3F3}, {"b", "d"});
            REQUIRE(iter == std::prev(map.end()));
        }
        SECTION("erase range")
        {
            auto iter = map.erase_range(std::next(map.begin()), std::next(map.begin(), 2));
            verify_map(map, {0xF0F0, 0xF2F2, 0xF3F3}, {"a", "c", "d"});
            REQUIRE(iter == std::next(map.begin()));

            iter = map.erase_range(std::next(map.begin()), map.end());
            verify_map(map, {0xF0F0}, {"a"});
            REQUIRE(iter == map.end());

            iter = map.erase_range(map.begin(), map.begin());
            verify_map(map, {0xF0F0}, {"a"});
            REQUIRE(iter == map.begin());

            iter = map.erase_range(map.begin(), map.end());
            verify_map(map, {}, {});
            REQUIRE(iter == map.end());
        }
        SECTION("erase all")
        {
            auto erased = map.erase_all(0xF2F2);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF3F3}, {"a", "b", "d"});
            REQUIRE(erased);

            erased = map.erase_all(0xF5F5);
            verify_map(map, {0xF0F0, 0xF1F1, 0xF3F3}, {"a", "b", "d"});
            REQUIRE(!erased);
        }
        SECTION("lookup")
        {
            // lookup of existing items already checked in verify_map()

            REQUIRE(!map.contains(0xF4F4));
            REQUIRE(map.try_lookup(0xF4F4) == nullptr);
            REQUIRE(map.find(0xF4F4) == map.end());
            REQUIRE(map.lower_bound(0xF4F4) == map.end());
            REQUIRE(map.upper_bound(0xF4F4) == map.end());

            auto range = map.equal_range(0xF4F4);
            REQUIRE(range.empty());
            REQUIRE(range.begin() == map.end());
            REQUIRE(range.end() == map.end());
        }
        SECTION("move constructor")
        {
            auto     data = iterator_to_pointer(map.key_begin());
            test_map other(std::move(map));
            verify_map(other, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
            verify_map(map, {}, {});
            REQUIRE(iterator_to_pointer(other.key_begin()) == data);

            SECTION("copy assignment")
            {
                map = other;
                verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
                verify_map(other, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
            }
            SECTION("move assignment")
            {
                map = std::move(other);
                verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
                verify_map(other, {}, {});
                REQUIRE(iterator_to_pointer(map.key_begin()) == data);
            }
            SECTION("swap")
            {
                swap(map, other);
                verify_map(map, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3}, {"a", "b", "c", "d"});
                verify_map(other, {}, {});
                REQUIRE(iterator_to_pointer(map.key_begin()) == data);
            }
        }
    }
}
