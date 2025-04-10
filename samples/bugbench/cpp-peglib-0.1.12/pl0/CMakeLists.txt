cmake_minimum_required(VERSION 3.1)
project(tcp)

enable_language(CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(..)
add_executable(pl0 pl0.cc)

find_package(LLVM REQUIRED CONFIG)
set(add_link_deps ${add_link_deps} LLVM)
target_include_directories(pl0 PUBLIC ${LLVM_INCLUDE_DIRS})
target_link_libraries(pl0 ${add_link_deps})
