# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(CommunicationPrimitives)

include_directories(.)

# add subprojects
add_subdirectory(utility)
add_subdirectory(data_structures)
#add_subdirectory(protocols)
#add_subdirectory(applications)

