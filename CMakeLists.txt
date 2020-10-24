# Specify the minimum version for CMake
cmake_minimum_required(VERSION 2.8)

# Project's name
project(CommunicationPrimitives)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

set (CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O0")

include_directories(/usr/include)
include_directories(/usr/include/uuid)
include_directories(/usr/include/libnl3)

include_directories(Yggdrasil/Yggdrasil-LowLevelLib/src/)
include_directories(Yggdrasil/Yggdrasil/)
include_directories(Yggdrasil/Yggdrasil/core/)
include_directories(Yggdrasil/Yggdrasil/core/utils)
include_directories(Yggdrasil/)

# Add subdirectories
add_subdirectory("Yggdrasil")
add_subdirectory("CommunicationPrimitives")
