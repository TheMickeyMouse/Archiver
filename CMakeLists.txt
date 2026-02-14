cmake_minimum_required(VERSION 4.0)
project(archive)

set(CMAKE_CXX_STANDARD 20)

add_executable(archive main.cpp
        Archive.cpp
        Archive.h)

add_subdirectory(Quasi)
target_link_libraries(archive PUBLIC Quasi)
target_include_directories(archive PUBLIC ${CMAKE_SOURCE_DIR}/Quasi/src)

if (MSVC)
    target_compile_options(archive PRIVATE
            "$<$<CONFIG:Release>:/O3>"
            "$<$<CONFIG:Release>:-DNDEBUG>"
    )
else()
    target_compile_options(archive PRIVATE
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:Release>:-DNDEBUG>"
        "$<$<CONFIG:Release>:-s>"
    )
endif()