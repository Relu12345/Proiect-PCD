cmake_minimum_required(VERSION 3.25)
project(Proiect_PCD C)

set(CMAKE_C_STANDARD 11)
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

add_executable(Proiect_PCD
        src/database.c)
target_link_libraries(Proiect_PCD ${PostgreSQL_LIBRARIES})