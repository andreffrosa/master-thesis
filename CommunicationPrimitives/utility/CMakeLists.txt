# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(utility)

include_directories(.)

file(GLOB_RECURSE SRC_FILES_UTILITY ./*.c)

#list(REMOVE_ITEM SRC_FILES_UTILITY ${PROJECT_SOURCE_DIR}/myUtils.c)

add_library(utility ${SRC_FILES_UTILITY})

#target_compile_definitions(data_structures PUBLIC WIRELESS_NETWORKS)
target_link_libraries(utility PUBLIC data_structures m uuid rt)
