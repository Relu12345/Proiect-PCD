cmake_minimum_required(VERSION 3.0)
project(Proect_PCD)

set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(client src/client.c)
add_executable(server src/server.c)

target_link_libraries(client ${OpenCV_LIBS})
target_link_libraries(server ${OpenCV_LIBS})

target_link_libraries(server pthread)

add_custom_target(client_python ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/client.py ${CMAKE_BINARY_DIR}/client.py
    COMMENT "Copying Python client to build directory"
)