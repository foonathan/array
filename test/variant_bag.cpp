// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/variant_bag.hpp>

#include <catch.hpp>

#include "equal_checker.hpp"
#include "leak_checker.hpp"

using namespace foonathan::array;

namespace
{
    template <typename T>
    struct my_type_tag
    {
    };

    template <typename T, class Bag>
    void verify_bag_impl(const Bag& bag, std::initializer_list<int> ids)
    {
        REQUIRE(bag.empty(type_t<T>{}) == (bag.size(type_t<T>{}) == 0u));
        REQUIRE(bag.size(type_t<T>{}) == size_type(ids.end() - ids.begin()));
        REQUIRE(bag.capacity(type_t<T>{}) >= bag.size(type_t<T>{}));
        REQUIRE(bag.capacity(type_t<T>{}) <= bag.max_size(type_t<T>{}));

        auto view = bag(type_t<T>{});
        REQUIRE(view.size() == bag.size(type_t<T>{}));
        REQUIRE(view.data() == iterator_to_pointer(bag.begin(type_t<T>{})));
        REQUIRE(view.data_end() == iterator_to_pointer(bag.end(type_t<T>{})));
        REQUIRE(view.data() == iterator_to_pointer(bag.cbegin(type_t<T>{})));
        REQUIRE(view.data_end() == iterator_to_pointer(bag.cend(type_t<T>{})));
        check_equal(view.begin(), view.end(), ids.begin(), ids.end(),
                    [](const T& test, int i) { return test.id == i; },
                    [&](const T& test) { FAIL_CHECK(std::hex << test.id); });
    }

    template <typename T, class Bag>
    void verify_bag_single(const Bag& bag, std::initializer_list<int> ids)
    {
        verify_bag_impl<T>(bag, ids);

        // copy constructor
        Bag copy(bag);
        verify_bag_impl<T>(copy, ids);
        REQUIRE(copy.capacity(type_t<T>{}) <= bag.capacity(type_t<T>{}));

        // shrink to fit
        auto old_cap = copy.capacity(type_t<T>{});
        copy.shrink_to_fit(type_t<T>{});
        verify_bag_impl<T>(copy, ids);
        REQUIRE(copy.capacity(type_t<T>{}) <= old_cap);

        // reserve
        copy.reserve(copy.size(type_t<T>{}) + 4u);
        REQUIRE(copy.capacity(type_t<T>{}) >= copy.size(type_t<T>{}) + 4u);
        verify_bag_impl<T>(copy, ids);

        // copy assignment
        copy.emplace(type_t<T>{}, 0xFFFF);
        copy = bag;
        verify_bag_impl<T>(copy, ids);
    }

    template <typename Tag>
    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
    };

    using first_t  = test_type<struct foo>;
    using second_t = test_type<struct bar>;

    using test_bag = variant_bag<block_storage_default, first_t, second_t>;

    void verify_bag(const test_bag& bag, std::initializer_list<int> first,
                    std::initializer_list<int> second)
    {
        verify_bag_single<first_t>(bag, first);
        verify_bag_single<second_t>(bag, second);

        // all type functions
        REQUIRE(bag.empty() == (bag.empty<first_t>() && bag.empty<second_t>()));
        REQUIRE(bag.size() == bag.size<first_t>() + bag.size<second_t>());

        // copy
        test_bag copy(bag);
        REQUIRE(copy.capacity<first_t>() <= bag.capacity<first_t>());
        REQUIRE(copy.capacity<second_t>() <= bag.capacity<second_t>());
        verify_bag_single<first_t>(copy, first);
        verify_bag_single<second_t>(copy, second);

        // reserve
        copy.reserve(bag.size() + 4u);
        REQUIRE(copy.capacity<first_t>() >= bag.size() + 4u);
        REQUIRE(copy.capacity<second_t>() >= bag.size() + 4u);
        verify_bag_single<first_t>(copy, first);
        verify_bag_single<second_t>(copy, second);

        // shrink_to_fit
        copy.shrink_to_fit();
        verify_bag_single<first_t>(copy, first);
        verify_bag_single<second_t>(copy, second);
    }
} // namespace

TEST_CASE("variant_bag", "[container]")
{
    leak_checker checker;

    SECTION("insertion")
    {
        test_bag bag;
        verify_bag(bag, {}, {});

        bag.emplace<first_t>(0xF0F0);
        verify_bag(bag, {0xF0F0}, {});

        bag.insert(first_t(0xF1F1));
        verify_bag(bag, {0xF0F0, 0xF1F1}, {});

        second_t test(0xF2F2);
        bag.insert(test);
        verify_bag(bag, {0xF0F0, 0xF1F1}, {0xF2F2});

        second_t array[] = {{0xF3F3}, {0xF4F4}};
        bag.insert(make_block_view(array));
        verify_bag(bag, {0xF0F0, 0xF1F1}, {0xF2F2, 0xF3F3, 0xF4F4});

        first_t range[] = {{0xF5F5}};
        bag.insert_range(std::begin(range), std::end(range));
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5}, {0xF2F2, 0xF3F3, 0xF4F4});

        int ids[] = {0xF6F6, 0xF7F7};
        bag.insert_range<first_t>(std::begin(ids), std::end(ids));
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5, 0xF6F6, 0xF7F7}, {0xF2F2, 0xF3F3, 0xF4F4});

        bag.insert_range<second_t>(std::begin(ids), std::end(ids));
        verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5, 0xF6F6, 0xF7F7},
                   {0xF2F2, 0xF3F3, 0xF4F4, 0xF6F6, 0xF7F7});

        SECTION("clear")
        {
            SECTION("first_t")
            {
                bag.clear<first_t>();
                verify_bag(bag, {}, {0xF2F2, 0xF3F3, 0xF4F4, 0xF6F6, 0xF7F7});
            }
            SECTION("second_t")
            {
                bag.clear<second_t>();
                verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5, 0xF6F6, 0xF7F7}, {});
            }
            SECTION("all")
            {
                bag.clear();
                verify_bag(bag, {}, {});
            }
        }
        SECTION("erase")
        {
            auto iter = std::prev(bag.end<first_t>());
            iter      = bag.erase(iter);
            verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5, 0xF6F6},
                       {0xF2F2, 0xF3F3, 0xF4F4, 0xF6F6, 0xF7F7});
            REQUIRE(iter == bag.end<first_t>());

            auto second_iter = std::prev(bag.end<second_t>(), 3);
            second_iter      = bag.erase(second_iter);
            verify_bag(bag, {0xF0F0, 0xF1F1, 0xF5F5, 0xF6F6}, {0xF2F2, 0xF3F3, 0xF7F7, 0xF6F6});
            REQUIRE(second_iter == std::prev(bag.end<second_t>(), 2));
        }
        SECTION("erase_range")
        {
            auto iter =
                bag.erase_range(std::next(bag.begin<first_t>()), std::prev(bag.end<first_t>()));
            verify_bag(bag, {0xF0F0, 0xF7F7}, {0xF2F2, 0xF3F3, 0xF4F4, 0xF6F6, 0xF7F7});
            REQUIRE(iter == std::prev(bag.end<first_t>()));
        }
    }
    SECTION("tag compilation")
    {
        variant_bag_tl<type_list<int, float>> bag;

#if FOONATHAN_ARRAY_HAS_VARIABLE_TEMPLATES
        bag.view(type<int>);
#endif

        bag.view(type_t<int>{});
        bag.view(my_type_tag<int>{});
        bag.view<int>();

        bag.empty();
        bag.empty<int>();

        bag.reserve(42);
        bag.reserve(type_t<int>{}, 42);
        bag.reserve(my_type_tag<int>{}, 42);
        bag.reserve<int>(42);

        bag.emplace(type_t<int>{}, 42);
        bag.emplace(my_type_tag<int>{}, 42);
        bag.emplace<int>(42);
    }
}

TEST_CASE("variant_bag insert iterator", "[container]")
{
    int   iarray[] = {1, 2, 3, 4, 5};
    float farray[] = {1.1f, 2.2f, 3.3f};

    variant_bag<block_storage_default, int, float> b;
    std::copy_if(std::begin(iarray), std::end(iarray), bag_inserter(b),
                 [](int i) { return i % 2 == 0; });
    std::copy(std::begin(farray), std::end(farray), bag_inserter(b));

    int iexpected[] = {2, 4};
    check_equal(b.begin<int>(), b.end<int>(), std::begin(iexpected), std::end(iexpected),
                [](int a, int b) { return a == b; }, [](int i) { FAIL_CHECK(i); });
    check_equal(b.begin<float>(), b.end<float>(), std::begin(farray), std::end(farray),
                [](float a, float b) { return a == b; }, [](float f) { FAIL_CHECK(f); });
}
