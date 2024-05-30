cmake_minimum_required(VERSION 3.0)
project(Proiect_PCD)

set (CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# Find OpenCV 4 package
find_package(OpenCV 4 REQUIRED)

# Find PostgreSQL package
find_package(PostgreSQL REQUIRED)

add_library(database SHARED src/database.c)
target_include_directories(database PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(database PRIVATE ${PostgreSQL_LIBRARIES})

add_library(login SHARED src/login.c)
target_include_directories(login PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(login PRIVATE ${PostgreSQL_LIBRARIES})

# Add executable for server
add_executable(server src/server.c src/base64.cpp src/interface_wrapper.cpp src/login.c src/image_prc_wrapper.cpp src/fistic_wrapper.cpp)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(server PRIVATE ${OpenCV_LIBS} ${PostgreSQL_LIBRARIES} pthread database pistache nlohmann_json::nlohmann_json login)

# Add executable for client
add_executable(client src/client.c src/interface_wrapper.cpp src/login.c src/image_prc_wrapper.cpp)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(client PRIVATE ${OpenCV_LIBS} database pistache login)

# Add executable for admin client
add_executable(admin src/admin.c)
target_include_directories(admin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(admin PRIVATE ${OpenCV_LIBS} database pistache login)