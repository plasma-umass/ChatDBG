cmake_minimum_required(VERSION 2.8)
project(peglint)
include_directories(..)
add_definitions("-std=c++11")
add_executable(peglint peglint.cc)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(peglint ${add_link_deps})
endif()
