// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <iostream>
#include <vector>

#include <foonathan/array/bag.hpp>
#include <foonathan/array/block_storage_new.hpp>

namespace array = foonathan::array;

int main()
{
    std::vector<int> vec(7);
    std::cout << vec.size() << ' ' << vec.capacity() << '\n';
}
