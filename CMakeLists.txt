cmake_minimum_required(VERSION 3.0)
project(Proect_PCD)

set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

add_executable(client src/client.c)
add_executable(server src/server.c)

target_link_libraries(server pthread)