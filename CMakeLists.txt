cmake_minimum_required(VERSION 2.8)
project(CREFILE)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

include_directories(./)
include_directories(tests)

set(CREFILE_HEADERS crefile.hpp)

add_executable(unittests tests/test.cpp tests/gtest/gtest-all.cc ${CREFILE_HEADERS})
