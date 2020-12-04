# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(broadcast_algorithm)

link_directories(${PROJECT_SOURCE_DIR}/retransmission_context/)
link_directories(${PROJECT_SOURCE_DIR}/retransmission_policy/)
link_directories(${PROJECT_SOURCE_DIR}/retransmission_delay/)

file(GLOB_RECURSE SRC_FILES_BROADCAST_ALGORITHM  ${PROJECT_SOURCE_DIR}/retransmission_context/*.c)
file(GLOB_RECURSE SRC_FILES_BROADCAST_ALGORITHM  ${PROJECT_SOURCE_DIR}/retransmission_policy/*.c)
file(GLOB_RECURSE SRC_FILES_BROADCAST_ALGORITHM  ${PROJECT_SOURCE_DIR}/retransmission_delay/*.c)
file(GLOB_RECURSE SRC_FILES_BROADCAST_ALGORITHM  ${PROJECT_SOURCE_DIR}/*.c)

#exclude files
#list(REMOVE_ITEM SRC_FILES_BROADCAST_ALGORITHM ${PROJECT_SOURCE_DIR}/retransmission_context/AHBPContext.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

add_library(broadcast_algorithm ${SRC_FILES_BROADCAST_ALGORITHM})

target_link_libraries(broadcast_algorithm PUBLIC discovery_algorithm discovery_framework utility data_structures m uuid rt)
