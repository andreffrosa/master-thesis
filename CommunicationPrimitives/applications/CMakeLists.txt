# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Project's name
project(cp_applications)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0 -lm -pthread -lnl-genl-3 -lnl-3 -luuid -lrt")
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lm -lnl-genl-3 -lnl-3 -luuid -lrt")

link_directories(${PROJECT_SOURCE_DIR})

#
add_executable(connect_test ${PROJECT_SOURCE_DIR}/connect.c ${PROJECT_SOURCE_DIR}/app_common.c)
target_link_libraries(connect_test utility yggcore ygginterfaces yggdatastruct ygglllib_obj)

# DISCOVERY

link_directories(${PROJECT_SOURCE_DIR}/discovery/)
add_executable(discovery_test ${PROJECT_SOURCE_DIR}/discovery/discovery_test.c ${PROJECT_SOURCE_DIR}/app_common.c)
target_link_libraries(discovery_test discovery_algorithm discovery_framework utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)


# BROADCAST

link_directories(${PROJECT_SOURCE_DIR}/broadcast/)
add_executable(broadcast_test ${PROJECT_SOURCE_DIR}/broadcast/broadcast_test.c ${PROJECT_SOURCE_DIR}/app_common.c)
target_link_libraries(broadcast_test broadcast broadcast_algorithm discovery_algorithm discovery_framework utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)

#add_executable(test_configs ${PROJECT_SOURCE_DIR}/broadcast/test_configs.c ${PROJECT_SOURCE_DIR}/broadcast/process_args.c)
#target_link_libraries(test_configs broadcast broadcast_algorithm discovery utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)

#add_executable(destroy_test ${PROJECT_SOURCE_DIR}/broadcast/destroy_test.c ${PROJECT_SOURCE_DIR}/broadcast/process_args.c)
#target_link_libraries(destroy_test broadcast broadcast_algorithm discovery utility yggprotos yggcore ygginterfaces yggdatastruct ygglllib_obj)
