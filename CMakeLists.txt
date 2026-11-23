cmake_minimum_required(VERSION 3.20)
project(Airport)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)
add_executable(
        json_activation_test
        tests/jsonlib_activation_test.cpp
)
target_include_directories(json_activation_test PRIVATE include)

add_executable(
        json_reader_test
        tests/json_reader_test.cpp
        src/data/json_reader.cpp
)
target_include_directories(json_reader_test PRIVATE include)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(
        excel_reader_test
        tests/excel_reader_test.cpp
        src/data/excel_reader.cpp
)
target_include_directories(excel_reader_test PRIVATE include)

add_executable(algorithm_test
        tests/algorithm_test.cpp
        src/algorithm.cpp
        src/data/json_reader.cpp
        src/data/excel_reader.cpp)
target_include_directories(algorithm_test PRIVATE include)

add_executable(gantt_test
                tests/gantt_test.cpp
                src/gantt.cpp
                src/algorithm.cpp
                src/data/json_reader.cpp
                src/data/excel_reader.cpp
                tests/gantt_test.cpp)
target_include_directories(gantt_test PRIVATE include)


add_executable(main
                main.cpp
                src/gantt.cpp
                src/algorithm.cpp
                src/data/json_reader.cpp
                src/data/excel_reader.cpp)