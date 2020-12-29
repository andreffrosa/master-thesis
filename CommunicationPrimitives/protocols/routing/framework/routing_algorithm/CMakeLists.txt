# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(routing_algorithm)

link_directories(${PROJECT_SOURCE_DIR}/forwarding_strategy/)
link_directories(${PROJECT_SOURCE_DIR}/routing_context/)

file(GLOB_RECURSE SRC_FILES_ROUTING_ALGORITHM  ${PROJECT_SOURCE_DIR}/forwarding_strategy/*.c)
file(GLOB_RECURSE SRC_FILES_ROUTING_ALGORITHM  ${PROJECT_SOURCE_DIR}/routing_context/*.c)
file(GLOB_RECURSE SRC_FILES_ROUTING_ALGORITHM  ${PROJECT_SOURCE_DIR}/*.c)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

add_library(routing_algorithm ${SRC_FILES_ROUTING_ALGORITHM})

target_link_libraries(routing_algorithm PUBLIC utility data_structures m uuid rt)
