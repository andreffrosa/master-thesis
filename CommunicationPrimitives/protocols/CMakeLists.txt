# Specify the minimum version for CMake

cmake_minimum_required(VERSION 2.8)

# Project's name
project(protocols)

add_subdirectory(discovery/framework)
add_subdirectory(broadcast/framework)
#add_subdirectory(routing/framework)
