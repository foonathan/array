// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/array/array.hpp>

#include <catch.hpp>

#include <foonathan/array/block_storage_embedded.hpp>
#include <foonathan/array/block_storage_new.hpp>
#include <foonathan/array/block_storage_sbo.hpp>

#include "equal_checker.hpp"
#include "leak_checker.hpp"

using namespace foonathan::array;

namespace
{
    struct test_type : leak_tracked
    {
        std::uint16_t id;

        test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
    };

    template <class BlockStorage>
    using test_array = array<test_type, BlockStorage>;

    template <class Array>
    void verify_array_impl(const Array& array, std::initializer_list<int> ids)
    {
        REQUIRE(array.empty() == (array.size() == 0u));
        REQUIRE(array.size() == size_type(ids.end() - ids.begin()));
        REQUIRE(array.capacity() >= array.size());
        REQUIRE(array.capacity() <= array.max_size());

        auto view = array_view<const test_type>(array);
        REQUIRE(view.size() == array.size());
        REQUIRE(view.data() == iterator_to_pointer(array.begin()));
        REQUIRE(view.data_end() == iterator_to_pointer(array.end()));
        REQUIRE(view.data() == iterator_to_pointer(array.cbegin()));
        REQUIRE(view.data_end() == iterator_to_pointer(array.cend()));
        check_equal(view.begin(), view.end(), ids.begin(), ids.end(),
                    [](const test_type& test, int i) { return test.id == i; },
                    [&](const test_type& test) { FAIL_CHECK(std::hex << test.id); });

        for (size_type i = 0u; i != array.size(); ++i)
            REQUIRE(array[i].id == ids.begin()[i]);

        if (!array.empty())
        {
            REQUIRE(array.front().id == *ids.begin());
            REQUIRE(array.back().id == *std::prev(ids.end()));
        }
    }

    template <class Array>
    void verify_array(const Array& array, std::initializer_list<int> ids)
    {
        verify_array_impl(array, ids);

        // copy constructor
        Array copy(array);
        verify_array_impl(copy, ids);
        REQUIRE(copy.capacity() <= array.capacity());

        // shrink to fit
        auto old_cap = copy.capacity();
        copy.shrink_to_fit();
        verify_array_impl(copy, ids);
        REQUIRE(copy.capacity() <= old_cap);

        // reserve
        copy.reserve(copy.size() + 4u);
        REQUIRE(copy.capacity() >= copy.size() + 4u);
        verify_array_impl(copy, ids);

        // copy assignment
        copy.push_back(0xFFFF);
        copy = array;
        verify_array_impl(copy, ids);

        // range assignment
        copy.push_back(0xFFFF);
        copy.assign_range(array.begin(), array.end());
        verify_array_impl(copy, ids);

        // block assignment
        copy.push_back(0xFFFF);
        copy.assign(array);
        verify_array_impl(copy, ids);
    }

    template <class Array>
    void array_test_impl()
    {
        SECTION("default ctor")
        {
            Array array;
            verify_array(array, {});

            // emplace_back/push_back
            array.emplace_back(0xF0F0);
            verify_array(array, {0xF0F0});

            array.push_back(test_type(0xF1F1));
            verify_array(array, {0xF0F0, 0xF1F1});

            test_type test(0xF2F2);
            array.push_back(test);
            verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2});

            SECTION("emplace/insert")
            {
                array.emplace(array.begin(), 0xF3F3);
                verify_array(array, {0xF3F3, 0xF0F0, 0xF1F1, 0xF2F2});

                array.insert(std::next(array.begin()), 0xF4F4);
                verify_array(array, {0xF3F3, 0xF4F4, 0xF0F0, 0xF1F1, 0xF2F2});

                test.id = 0xF5F5;
                array.insert(std::prev(array.end()), test);
                verify_array(array, {0xF3F3, 0xF4F4, 0xF0F0, 0xF1F1, 0xF5F5, 0xF2F2});
            }
            SECTION("append")
            {
                test_type tests[] = {{0xF3F3}, {0xF4F4}, {0xF5F5}};
                array.append(tests);
                verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});

                array.append_range(std::begin(tests), std::end(tests));
                verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4,
                                     0xF5F5});

                SECTION("pop_back/erase")
                {
                    array.pop_back();
                    verify_array(array,
                                 {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4});

                    array.erase(array.begin());
                    verify_array(array, {0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3, 0xF4F4});

                    array.erase(array.begin() + 3);
                    verify_array(array, {0xF1F1, 0xF2F2, 0xF3F3, 0xF5F5, 0xF3F3, 0xF4F4});
                }
                SECTION("erase range")
                {
                    array.erase_range(std::prev(array.end(), 3), array.end());
                    verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5});

                    array.erase_range(std::next(array.begin()), std::prev(array.end()));
                    verify_array(array, {0xF0F0, 0xF5F5});

                    array.erase_range(array.begin(), array.begin());
                    verify_array(array, {0xF0F0, 0xF5F5});
                }
                SECTION("move constructor")
                {
                    using is_embedded = embedded_storage<typename Array::block_storage>;

                    auto  data = iterator_to_pointer(array.begin());
                    Array other(std::move(array));
                    verify_array(other, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                         0xF4F4, 0xF5F5});
                    verify_array(array, {});
                    if (!is_embedded{})
                        REQUIRE(iterator_to_pointer(other.begin()) == data);

                    SECTION("copy assignment")
                    {
                        array = other;
                        verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                             0xF4F4, 0xF5F5});
                        verify_array(other, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                             0xF4F4, 0xF5F5});
                    }
                    SECTION("move assignment")
                    {
                        array = std::move(other);
                        verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                             0xF4F4, 0xF5F5});
                        verify_array(other, {});
                        if (!is_embedded{})
                            REQUIRE(iterator_to_pointer(array.begin()) == data);
                    }
                    SECTION("input_view assignment")
                    {
                        array =
                            input_view<test_type, typename Array::block_storage>(std::move(other));
                        verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                             0xF4F4, 0xF5F5});
                        verify_array(other, {});
                        if (!is_embedded{})
                            REQUIRE(iterator_to_pointer(array.begin()) == data);
                    }
                    SECTION("swap")
                    {
                        swap(array, other);
                        verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF3F3,
                                             0xF4F4, 0xF5F5});
                        verify_array(other, {});
                        if (!is_embedded{})
                            REQUIRE(iterator_to_pointer(array.begin()) == data);
                    }
                }
            }
            SECTION("multi insert")
            {
                test_type tests[] = {{0xF3F3}, {0xF4F4}, {0xF5F5}};
                array.insert(std::next(array.begin()), tests);
                verify_array(array, {0xF0F0, 0xF3F3, 0xF4F4, 0xF5F5, 0xF1F1, 0xF2F2});

                array.insert_range(std::prev(array.end()), std::begin(tests), std::end(tests));
                verify_array(array, {0xF0F0, 0xF3F3, 0xF4F4, 0xF5F5, 0xF1F1, 0xF3F3, 0xF4F4, 0xF5F5,
                                     0xF2F2});
            }
            SECTION("clear")
            {
                auto old_cap = array.capacity();
                array.clear();
                REQUIRE(old_cap == array.capacity());
                verify_array(array, {});
            }
        }
        SECTION("input_view ctor")
        {
            Array array{
                {test_type(0xF0F0), test_type(0xF1F1), test_type(0xF2F2), test_type(0xF3F3)}};
            verify_array(array, {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3});
        }
    }
} // namespace

TEST_CASE("array block_storage_new", "[container]")
{
    array_test_impl<test_array<block_storage_new<default_growth>>>();
}

TEST_CASE("array block_storage_embedded", "[container]")
{
    array_test_impl<test_array<block_storage_embedded<15 * sizeof(test_type)>>>();
}

TEST_CASE("array block_storage_sbo", "[container]")
{
    array_test_impl<test_array<block_storage_sbo<5 * sizeof(test_type), block_storage_default>>>();
}
