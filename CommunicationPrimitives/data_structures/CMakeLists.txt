# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(data_structures)

#include_directories(.)

file(GLOB_RECURSE SRC_FILES_DATA_STRUCT *.c)
add_library(data_structures ${SRC_FILES_DATA_STRUCT})

#target_compile_definitions(data_structures PUBLIC WIRELESS_NETWORKS)
target_link_libraries(data_structures PUBLIC utility yggdatastruct m uuid rt)
