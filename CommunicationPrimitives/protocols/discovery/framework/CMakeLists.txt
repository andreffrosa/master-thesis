# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(discovery_framework)

add_subdirectory(discovery_algorithm)

file(GLOB SRC_FILES_DISCOVERY_FRAMEWORK LIST_DIRECTORIES false ${PROJECT_SOURCE_DIR}/*.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D DEBUG_DISCOVERY -g -Wall -O0")

add_library(discovery_framework ${SRC_FILES_DISCOVERY_FRAMEWORK})

#target_compile_definitions(data_structures PUBLIC WIRELESS_NETWORKS)
target_link_libraries(discovery_framework PUBLIC discovery_algorithm utility data_structures m uuid rt)
