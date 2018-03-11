// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <iostream>

#include <foonathan/array/bag.hpp>
#include <foonathan/array/block_storage_new.hpp>

namespace array = foonathan::array;

int main()
{
    using bag = array::bag<int, array::block_storage_new<int, array::default_growth>>;

    bag b;
    b.insert(1);
    b.insert(2);
    b.insert(3);
    b.insert(4);

    for (auto el : b)
        std::cout << el << ' ';
    std::cout << '\n';

    auto iter = std::find(b.begin(), b.end(), 2);
    b.erase(iter);

    for (auto el : b)
        std::cout << el << ' ';
    std::cout << '\n';

    b.insert(5);

    for (auto el : b)
        std::cout << el << ' ';
    std::cout << '\n';
}
