cmake_minimum_required(VERSION 3.20)
project(cpu16 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(cpu16_core
    src/isa.cpp
    src/cpu.cpp
    src/assembler.cpp
    src/report.cpp
)
target_include_directories(cpu16_core PUBLIC include)
if(MINGW)
    target_link_options(cpu16_core PUBLIC -static)
endif()

add_executable(cpu16 src/main.cpp)
target_link_libraries(cpu16 PRIVATE cpu16_core)

option(CPU16_BUILD_TESTS "Build the cpu16 unit test suite" ON)
if(CPU16_BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)
    add_executable(cpu16_tests
        tests/test_isa.cpp
        tests/test_cpu.cpp
        tests/test_assembler.cpp
        tests/test_programs.cpp
    )
    target_link_libraries(cpu16_tests PRIVATE cpu16_core Catch2::Catch2WithMain)

    enable_testing()
    include(CTest)
    add_test(NAME cpu16_tests COMMAND cpu16_tests)
endif()
