cmake_minimum_required(VERSION 3.9)
project(array)

set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/array/memory_block.hpp
    )
add_library(foonathan_array INTERFACE)
target_sources(foonathan_array INTERFACE ${header_files})
target_include_directories(foonathan_array INTERFACE include)

add_executable(foonathan_array_compile main.cpp)
target_link_libraries(foonathan_array_compile PUBLIC foonathan_array)
set_target_properties(foonathan_array_compile PROPERTIES CXX_STANDARD 11)
