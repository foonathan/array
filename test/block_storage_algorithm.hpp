// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED
#define FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED

// the test cases for the block storage algorithms are all templated on the storage
// that way they can be used to test both the algorithms and the BlockStorage's

#include <foonathan/array/block_storage.hpp>

#include <catch.hpp>

#include "leak_checker.hpp"

// TODO: big argument propagation test
// TODO: exception safety test
namespace test
{
    using namespace foonathan::array;

    template <class BlockStorage>
    void test_clear_and_shrink(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
        };

        BlockStorage storage(arguments);
        auto         empty_size = storage.block().size();

        auto size = 4u * sizeof(test_type);
        storage.reserve(size, block_view<test_type>());

        auto end         = uninitialized_fill(storage.block(), 4u, test_type());
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        REQUIRE(storage.block().size() >= size);
        clear_and_shrink(storage, constructed);
        REQUIRE(storage.block().size() == empty_size);
    }

    template <class BlockStorage>
    void test_clear_and_reserve(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
        };

        BlockStorage storage(arguments);
        auto         empty_size = storage.block().size();

        auto size = 4u * sizeof(test_type);
        storage.reserve(size, block_view<test_type>());

        auto end         = uninitialized_fill(storage.block(), 4u, test_type());
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        SECTION("bigger")
        {
            REQUIRE(storage.block().size() >= size);
            auto result = clear_and_reserve(storage, constructed, 2 * size);
            REQUIRE(storage.block().size() >= 2 * size);
            REQUIRE(result == storage.block().begin());

            constructed = block_view<test_type>();
        }
        SECTION("smaller")
        {
            REQUIRE(storage.block().size() >= size);
            auto result = clear_and_reserve(storage, constructed, size / 2);
            REQUIRE(storage.block().size() >= size / 2);
            REQUIRE(result == storage.block().begin());

            constructed = block_view<test_type>();
        }

        auto cur_size = storage.block().size();
        auto result   = clear_and_reserve(storage, constructed, 0u);
        REQUIRE(storage.block().size() == cur_size);
        REQUIRE(result == storage.block().begin());
    }

    template <class BlockStorage>
    void test_move_to_front(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
        };

        BlockStorage storage(arguments);
        storage.reserve(8u * sizeof(test_type), block_view<test_type>());

        int                   array[] = {0xF0F0, 0xF1F1};
        block_view<test_type> new_constructed;
        SECTION("no overlap")
        {
            auto end =
                uninitialized_copy_convert<test_type>(std::begin(array), std::end(array),
                                                      memory_block(storage.block().begin() + 4u,
                                                                   storage.block().end()));
            auto constructed =
                block_view<test_type>(memory_block(storage.block().begin() + 4u, end));

            new_constructed = move_to_front(storage, constructed);
        }
        SECTION("overlap")
        {
            auto end = uninitialized_copy_convert<test_type>(std::begin(array), std::end(array),
                                                             memory_block(storage.block().begin()
                                                                              + sizeof(test_type),
                                                                          storage.block().end()));
            auto constructed = block_view<test_type>(
                memory_block(storage.block().begin() + sizeof(test_type), end));

            new_constructed = move_to_front(storage, constructed);
        }

        if (!new_constructed.empty())
        {
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 2u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF1F1);

            auto new_new_constructed = move_to_front(storage, new_constructed);
            REQUIRE(new_new_constructed.data() == new_constructed.data());
            REQUIRE(new_new_constructed.size() == new_constructed.size());

            destroy_range(new_constructed.begin(), new_constructed.end());
        }
    }

    template <class BlockStorage>
    void test_assign(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
        };

        BlockStorage storage(arguments);

        auto size = 4u * sizeof(test_type);
        storage.reserve(size, block_view<test_type>());
        auto cur_size = storage.block().size();

        auto end         = uninitialized_fill(storage.block(), 3u, test_type(0xFFFF));
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        int values[] = {0xF0F0, 0xF1F1, 0xF2F2, 0xF3F3, 0xF4F4, 0xF5F5, 0xF6F6, 0xF7F7};

        SECTION("less than constructed")
        {
            auto new_constructed =
                assign(storage, constructed, std::begin(values), std::begin(values) + 2);
            REQUIRE(storage.block().size() == cur_size);

            REQUIRE(new_constructed.data() == constructed.data());
            REQUIRE(new_constructed.size() == 2u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF1F1);

            constructed = new_constructed;
        }
        SECTION("less than size")
        {
            auto new_constructed =
                assign(storage, constructed, std::begin(values), std::begin(values) + 4);
            REQUIRE(storage.block().size() == cur_size);

            REQUIRE(new_constructed.data() == constructed.data());
            REQUIRE(new_constructed.size() == 4u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF1F1);
            REQUIRE(new_constructed.data()[2].id == 0xF2F2);
            REQUIRE(new_constructed.data()[3].id == 0xF3F3);

            constructed = new_constructed;
        }
        SECTION("more than size")
        {
            auto new_constructed =
                assign(storage, constructed, std::begin(values), std::begin(values) + 8);
            REQUIRE(storage.block().size() >= 8 * sizeof(test_type));

            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 8u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF1F1);
            REQUIRE(new_constructed.data()[2].id == 0xF2F2);
            REQUIRE(new_constructed.data()[3].id == 0xF3F3);
            REQUIRE(new_constructed.data()[4].id == 0xF4F4);
            REQUIRE(new_constructed.data()[5].id == 0xF5F5);
            REQUIRE(new_constructed.data()[6].id == 0xF6F6);
            REQUIRE(new_constructed.data()[7].id == 0xF7F7);

            constructed = new_constructed;
        }

        cur_size             = storage.block().size();
        auto new_constructed = assign(storage, constructed, std::begin(values), std::begin(values));
        REQUIRE(storage.block().size() == cur_size);

        REQUIRE(new_constructed.data() == constructed.data());
        REQUIRE(new_constructed.size() == 0u);
    }

    template <class BlockStorage>
    void test_fill(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
        };

        BlockStorage storage(arguments);

        auto size = 4u * sizeof(test_type);
        storage.reserve(size, block_view<test_type>());
        auto cur_size = storage.block().size();

        auto end         = uninitialized_fill(storage.block(), 3u, test_type(0xFFFF));
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        SECTION("empty") {}
        SECTION("less than constructed")
        {
            auto new_constructed = fill(storage, constructed, 2u, test_type(0xF0F0));
            REQUIRE(storage.block().size() == cur_size);

            REQUIRE(new_constructed.data() == constructed.data());
            REQUIRE(new_constructed.size() == 2u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);

            constructed = new_constructed;
        }
        SECTION("less than size")
        {
            auto new_constructed = fill(storage, constructed, 4u, test_type(0xF0F0));
            REQUIRE(storage.block().size() == cur_size);

            REQUIRE(new_constructed.data() == constructed.data());
            REQUIRE(new_constructed.size() == 4u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);

            constructed = new_constructed;
        }
        SECTION("more than size")
        {
            auto new_constructed = fill(storage, constructed, 8u, test_type(0xF0F0));
            REQUIRE(storage.block().size() >= 8 * sizeof(test_type));

            REQUIRE(new_constructed.data()
                    == to_pointer<test_type>(new_constructed.block().begin()));
            REQUIRE(new_constructed.size() == 8u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);
            REQUIRE(new_constructed.data()[4].id == 0xF0F0);
            REQUIRE(new_constructed.data()[5].id == 0xF0F0);
            REQUIRE(new_constructed.data()[6].id == 0xF0F0);
            REQUIRE(new_constructed.data()[7].id == 0xF0F0);

            constructed = new_constructed;
        }

        cur_size             = storage.block().size();
        auto new_constructed = fill(storage, constructed, 0u, test_type(0xF0F0));
        REQUIRE(storage.block().size() == cur_size);

        REQUIRE(new_constructed.data() == constructed.data());
        REQUIRE(new_constructed.size() == 0u);
    }

    template <class BlockStorage>
    void test_move_assign(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
        };

        BlockStorage storage(arguments);
        auto         empty_size = storage.block().size();

        storage.reserve(4u * sizeof(test_type), block_view<test_type>());

        auto end         = uninitialized_fill(storage.block(), 3u, test_type(0xFFFF));
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        SECTION("less than constructed")
        {
            BlockStorage other(arguments);
            other.reserve(2u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 2u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed =
                move_assign(storage, constructed, std::move(other), other_constructed);

            REQUIRE(other.block().size() == empty_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 2u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);

            constructed = new_constructed;
        }
        SECTION("less than size")
        {
            BlockStorage other(arguments);
            other.reserve(4u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 4u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed =
                move_assign(storage, constructed, std::move(other), other_constructed);

            REQUIRE(other.block().size() == empty_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 4u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);

            constructed = new_constructed;
        }
        SECTION("more than size")
        {
            BlockStorage other(arguments);
            other.reserve(8u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 8u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed =
                move_assign(storage, constructed, std::move(other), other_constructed);

            REQUIRE(other.block().size() == empty_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 8u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);
            REQUIRE(new_constructed.data()[4].id == 0xF0F0);
            REQUIRE(new_constructed.data()[5].id == 0xF0F0);
            REQUIRE(new_constructed.data()[6].id == 0xF0F0);
            REQUIRE(new_constructed.data()[7].id == 0xF0F0);

            constructed = new_constructed;
        }

        BlockStorage other(arguments);

        auto new_constructed =
            move_assign(storage, constructed, std::move(other), block_view<test_type>());

        REQUIRE(other.block().size() == empty_size);

        REQUIRE(storage.block().size() == empty_size);
        REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
        REQUIRE(new_constructed.size() == 0u);
    }

    template <class BlockStorage>
    void test_copy_assign(const typename BlockStorage::arg_type& arguments)
    {
        leak_checker checker;
        struct test_type : leak_tracked
        {
            std::uint16_t id;

            test_type(int i) : id(static_cast<std::uint16_t>(i)) {}
        };

        BlockStorage storage(arguments);
        auto         empty_size = storage.block().size();

        storage.reserve(4u * sizeof(test_type), block_view<test_type>());

        auto end         = uninitialized_fill(storage.block(), 3u, test_type(0xFFFF));
        auto constructed = block_view<test_type>(memory_block(storage.block().begin(), end));

        SECTION("less than constructed")
        {
            BlockStorage other(arguments);
            other.reserve(2u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 2u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed = copy_assign(storage, constructed, other, other_constructed);

            REQUIRE(other.block().size() == other_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 2u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);

            constructed = new_constructed;

            destroy_range(other_constructed.begin(), other_constructed.end());
        }
        SECTION("less than size")
        {
            BlockStorage other(arguments);
            other.reserve(4u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 4u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed = copy_assign(storage, constructed, other, other_constructed);

            REQUIRE(other.block().size() == other_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 4u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);

            constructed = new_constructed;

            destroy_range(other_constructed.begin(), other_constructed.end());
        }
        SECTION("more than size")
        {
            BlockStorage other(arguments);
            other.reserve(8u * sizeof(test_type), block_view<test_type>());
            auto other_size = other.block().size();

            auto other_end = uninitialized_fill(other.block(), 8u, test_type(0xF0F0));
            auto other_constructed =
                block_view<test_type>(memory_block(other.block().begin(), other_end));

            auto new_constructed = copy_assign(storage, constructed, other, other_constructed);

            REQUIRE(other.block().size() == other_size);

            REQUIRE(storage.block().size() == other_size);
            REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
            REQUIRE(new_constructed.size() == 8u);
            REQUIRE(new_constructed.data()[0].id == 0xF0F0);
            REQUIRE(new_constructed.data()[1].id == 0xF0F0);
            REQUIRE(new_constructed.data()[2].id == 0xF0F0);
            REQUIRE(new_constructed.data()[3].id == 0xF0F0);
            REQUIRE(new_constructed.data()[4].id == 0xF0F0);
            REQUIRE(new_constructed.data()[5].id == 0xF0F0);
            REQUIRE(new_constructed.data()[6].id == 0xF0F0);
            REQUIRE(new_constructed.data()[7].id == 0xF0F0);

            constructed = new_constructed;

            destroy_range(other_constructed.begin(), other_constructed.end());
        }

        BlockStorage other(arguments);

        auto new_constructed = copy_assign(storage, constructed, other, block_view<test_type>());

        REQUIRE(other.block().size() == empty_size);

        REQUIRE(storage.block().size() == empty_size);
        REQUIRE(new_constructed.data() == to_pointer<test_type>(storage.block().begin()));
        REQUIRE(new_constructed.size() == 0u);
    }
}

#endif // FOONATHAN_ARRAY_BLOCK_STORAGE_ALGORITHM_HPP_INCLUDED
