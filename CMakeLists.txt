cmake_minimum_required(VERSION 4.0)
project(Query)

set(CMAKE_CXX_STANDARD 20)

add_executable(Query main.cpp
        Archive.cpp
        Archive.h)
add_subdirectory(Quasi)
target_link_libraries(Query PUBLIC Quasi)
target_include_directories(Query PUBLIC ${CMAKE_SOURCE_DIR}/Quasi/src)
