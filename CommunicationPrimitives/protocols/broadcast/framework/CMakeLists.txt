# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(broadcast)

add_subdirectory(pending_messages)
add_subdirectory(broadcast_algorithm)

file(GLOB SRC_FILES_BROADCAST_FRAMEWORK LIST_DIRECTORIES false ${PROJECT_SOURCE_DIR}/*.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D DEBUG_BROADCAST -g -Wall -O0")

add_library(broadcast ${SRC_FILES_BROADCAST_FRAMEWORK})

#target_compile_definitions(data_structures PUBLIC WIRELESS_NETWORKS)
target_link_libraries(broadcast PUBLIC pending_messages broadcast_algorithm discovery_algorithm discovery_framework utility data_structures m uuid rt)
