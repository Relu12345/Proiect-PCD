# cmake_minimum_required(VERSION 3.0)
# project(Proect_PCD)

# set(CMAKE_BUILD_TYPE Debug)

# include_directories(include)

# find_package(OpenCV REQUIRED)
# include_directories(${OpenCV_INCLUDE_DIRS})

# add_executable(client src/client.c)
# add_executable(server src/server.c)

# target_link_libraries(client ${OpenCV_LIBS})
# target_link_libraries(server ${OpenCV_LIBS})

# target_link_libraries(server pthread)

# add_custom_target(client_python ALL
#     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/client.py ${CMAKE_BINARY_DIR}/client.py
#     COMMENT "Copying Python client to build directory"
# )

cmake_minimum_required(VERSION 3.0)
project(OpenCV_C_Wrapper)

# Find OpenCV 4 package
find_package(OpenCV 4 REQUIRED)

# Add executable for server
add_executable(server src/server.c src/opencv_wrapper.cpp src/login.c)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(server PRIVATE ${OpenCV_LIBS} pthread)

# Add executable for client
add_executable(client src/client.c src/opencv_wrapper.cpp src/login.c)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(client PRIVATE ${OpenCV_LIBS})