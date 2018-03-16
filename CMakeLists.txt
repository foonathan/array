cmake_minimum_required(VERSION 3.9)
project(array)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Werror")

set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/array_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/bag.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_embedded.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_storage_new.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/block_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/byte_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/config.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/contiguous_iterator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/growth_policy.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/input_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/memory_block.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/pointer_iterator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/raw_storage.hpp
    )
add_library(foonathan_array INTERFACE)
target_sources(foonathan_array INTERFACE ${header_files})
target_include_directories(foonathan_array INTERFACE include)

add_subdirectory(test)

add_executable(foonathan_array_compile main.cpp)
target_link_libraries(foonathan_array_compile PUBLIC foonathan_array)
set_target_properties(foonathan_array_compile PROPERTIES CXX_STANDARD 11)
