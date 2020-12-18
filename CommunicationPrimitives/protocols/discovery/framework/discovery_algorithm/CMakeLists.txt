# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(discovery_algorithm)

file(GLOB_RECURSE SRC_FILES_DISCOVERY_ALGORITHM ${PROJECT_SOURCE_DIR}/*.c)

#exclude files

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

add_library(discovery_algorithm ${SRC_FILES_DISCOVERY_ALGORITHM})
target_link_libraries(discovery_algorithm PUBLIC discovery_framework utility data_structures m uuid rt)
