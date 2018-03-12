# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD
         https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
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

set(tests
        block_storage_args.cpp
        contiguous_iterator.cpp
        growth_policy.cpp
        memory_block.cpp
        pointer_iterator.cpp
        raw_storage.cpp)

add_executable(foonathan_array_test
                test.cpp
                leak_checker.hpp
                ${tests})
target_include_directories(foonathan_array_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(foonathan_array_test PUBLIC foonathan_array)
set_target_properties(foonathan_array_test PROPERTIES CXX_STANDARD 11)

add_test(NAME test COMMAND foonathan_array_test)
