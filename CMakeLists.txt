# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.8)
project(foonathan_array VERSION 0.0.0)

# source files
set(detail_header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/detail/all_of.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/detail/is_trivial.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/detail/swappable.hpp
    )
set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/array.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/array_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/bag.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_algorithm.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_allocator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_default.hpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/small_array.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/small_bag.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/variant_bag.hpp
    )

# main target
add_library(foonathan_array INTERFACE)
add_library(foonathan::foonathan_array ALIAS foonathan_array)
target_sources(foonathan_array INTERFACE $<BUILD_INTERFACE:${detail_header_files} ${header_files}>)
target_compile_features(foonathan_array INTERFACE cxx_std_11)
target_include_directories(foonathan_array SYSTEM INTERFACE
                               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                               $<INSTALL_INTERFACE:include/>)

# installation
if(NOT dependency_via_submodule)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(foonathan_array-config-version.cmake COMPATIBILITY ExactVersion)
    install(TARGETS foonathan_array EXPORT foonathan_array_targets
            INCLUDES DESTINATION include)
    install(EXPORT foonathan_array_targets
            DESTINATION lib/cmake/foonathan_array
            FILE foonathan_array-targets.cmake
            NAMESPACE foonathan::)
    install(DIRECTORY include/
            DESTINATION include)
    install(FILES foonathan_array-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/foonathan_array-config-version.cmake
            DESTINATION lib/cmake/foonathan_array)
else()
    message(STATUS "Dependency installed via submodule, installation unavailable")
endif()

# subdirectories
option(FOONATHAN_ARRAY_BUILD_TEST "build tests of foonathan/array" OFF)
if(${FOONATHAN_ARRAY_BUILD_TEST} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    enable_testing()
    add_subdirectory(test)
endif()
