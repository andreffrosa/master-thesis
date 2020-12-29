# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(routing)

add_subdirectory(routing_algorithm)

file(GLOB SRC_FILES_ROUTING_FRAMEWORK LIST_DIRECTORIES false ${PROJECT_SOURCE_DIR}/*.c)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D DEBUG_ROUTING -g -Wall -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

add_library(routing ${SRC_FILES_ROUTING_FRAMEWORK})

#target_compile_definitions(data_structures PUBLIC WIRELESS_NETWORKS)
target_link_libraries(routing PUBLIC routing_algorithm broadcast broadcast_algorithm discovery_framework discovery_algorithm utility data_structures m uuid rt)
