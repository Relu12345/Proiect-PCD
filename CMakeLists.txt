cmake_minimum_required(VERSION 3.0)
project(Proiect_PCD)

# Find OpenCV 4 package
find_package(OpenCV 4 REQUIRED)

# Find PostgreSQL package
find_package(PostgreSQL REQUIRED)

add_library(database SHARED src/database.c)
target_include_directories(database PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(database PRIVATE ${PostgreSQL_LIBRARIES})

# Add executable for server
add_executable(server src/server.c src/interface_wrapper.cpp src/login.c src/image_prc_wrapper.cpp)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(server PRIVATE ${OpenCV_LIBS} ${PostgreSQL_LIBRARIES} pthread database)

# Add executable for client
add_executable(client src/client.c src/interface_wrapper.cpp src/login.c src/image_prc_wrapper.cpp)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(client PRIVATE ${OpenCV_LIBS} database)

# Add executable for admin client
add_executable(admin src/admin.c)
target_include_directories(admin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(admin PRIVATE ${OpenCV_LIBS} database)