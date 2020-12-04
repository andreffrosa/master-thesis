# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(pending_messages)

file(GLOB_RECURSE SRC_FILES_PENDING_MESSAGES ${PROJECT_SOURCE_DIR}/*.c)

#exclude files
list(REMOVE_ITEM SRC_FILES_PENDING_MESSAGES ${PROJECT_SOURCE_DIR}/messages.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

add_library(pending_messages ${SRC_FILES_PENDING_MESSAGES})

target_link_libraries(pending_messages PUBLIC discovery_algorithm discovery_framework utility data_structures m uuid rt)
