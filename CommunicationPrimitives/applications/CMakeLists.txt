# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Project's name
project(cp_applications)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0 -lm -pthread -lnl-genl-3 -lnl-3 -luuid -lrt")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lm -lnl-genl-3 -lnl-3 -luuid -lrt")

link_directories(${PROJECT_SOURCE_DIR})

#
add_executable(connect_test ${PROJECT_SOURCE_DIR}/connect.c ${PROJECT_SOURCE_DIR}/app_common.c)
target_link_libraries(connect_test utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)

add_executable(dijkstra_test ${PROJECT_SOURCE_DIR}/aux/dijkstra_test.c)
target_link_libraries(dijkstra_test utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)

add_executable(tlv_test ${PROJECT_SOURCE_DIR}/aux/tlv_test.c)
target_link_libraries(tlv_test utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)


# DISCOVERY

link_directories(${PROJECT_SOURCE_DIR}/discovery/)
add_executable(discovery_test ${PROJECT_SOURCE_DIR}/discovery/discovery_test.c ${PROJECT_SOURCE_DIR}/app_common.c)
target_link_libraries(discovery_test discovery_framework discovery_algorithm utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)
