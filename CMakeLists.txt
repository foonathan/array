# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(array)

set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/array.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/array_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/bag.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_allocator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_embedded.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_heap.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_heap_sbo.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_new.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_sbo.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/byte_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/config.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/contiguous_iterator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/flat_set.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/flat_map.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/growth_policy.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/input_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/key_compare.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/memory_block.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/pointer_iterator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/raw_storage.hpp
    )
add_library(foonathan_array INTERFACE)
target_sources(foonathan_array INTERFACE ${header_files})
target_include_directories(foonathan_array INTERFACE include)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(test)
endif()
