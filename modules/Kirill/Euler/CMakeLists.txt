cmake_minimum_required(VERSION 2.8)
project(Euler)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()

add_definitions(${CMAKE_C_FLAGS})

set( SOURCE_FILES_EX1 euler.c )
add_executable(${PROJECT_NAME} ${SOURCE_FILES_EX1})

set( SOURCE_FILES_EX2 euler_complex.c )
add_executable(${PROJECT_NAME}_complex ${SOURCE_FILES_EX1})

