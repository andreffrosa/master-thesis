# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Project's name
project(cp_applications)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0 -lm -pthread -lnl-genl-3 -lnl-3 -luuid -lrt")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lm -lnl-genl-3 -lnl-3 -luuid -lrt")

# DISCOVERY

link_directories(${PROJECT_SOURCE_DIR}/discovery/)
add_executable(discovery_test ${PROJECT_SOURCE_DIR}/discovery/discovery_test.c)
target_link_libraries(discovery_test discovery_algorithm discovery_framework utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)

