# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# get Catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD
         https://raw.githubusercontent.com/catchorg/Catch2/master/single_include/catch2/catch.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
         STATUS status
         LOG log)

    list(GET status 0 status_code)
    list(GET status 1 status_string)

    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "error downloading catch: ${status_string}"
                "${log}")
    endif()
endif()

# unit tests
set(tests
    array.cpp
    array_view.cpp
    bag.cpp
    block_storage.cpp
    block_storage_algorithm.hpp
    block_storage_allocator.cpp
    block_storage_embedded.cpp
    block_storage_new.cpp
    block_storage_sbo.cpp
    block_view.cpp
    byte_view.cpp
    contiguous_iterator.cpp
    flat_map.cpp
    flat_set.cpp
    growth_policy.cpp
    input_view.cpp
    key_compare.cpp
    memory_block.cpp
    pointer_iterator.cpp
    raw_storage.cpp
    small.cpp
    variant_bag.cpp)

add_executable(foonathan_array_test
                test.cpp
                equal_checker.hpp
                leak_checker.hpp
                ${tests})
target_include_directories(foonathan_array_test PUBLIC
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}/../include
                           SYSTEM PUBLIC
                               ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(foonathan_array_test PUBLIC cxx_std_11)
target_compile_options(foonathan_array_test PUBLIC
                       # clang/GCC warnings
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                       -pedantic-errors -Werror -Wall -Wextra -Wconversion -Wsign-conversion>
                       # disable noexcept type warning on GCC
                       $<$<CXX_COMPILER_ID:GNU>: -Wno-noexcept-type>
                       # MSVC warnings
                       $<$<CXX_COMPILER_ID:MSVC>:
                       /WX /W4 /D_SCL_SECURE_NO_WARNINGS>)

add_test(NAME test COMMAND foonathan_array_test)
