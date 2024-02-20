cmake_minimum_required(VERSION 3.16)

# Setting the toolchain file
set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
project(chatdbg-windbg-support)

# Set C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add your sources
add_library(chatdbg SHARED chatdbg.cpp)

# Add include of source directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add Windows Debugger SDK includes and libraries
target_include_directories(chatdbg PRIVATE "C:/Program Files (x86)/Windows Kits/10/Debuggers/include")
target_link_libraries(chatdbg PRIVATE 
    "C:/Program Files (x86)/Windows Kits/10/Debuggers/lib/%VSCMD_ARG_HOST_ARCH%/dbgeng.lib" 
    "C:/Program Files (x86)/Windows Kits/10/Debuggers/lib/%VSCMD_ARG_HOST_ARCH%/dbghelp.lib")

# Include vcpkg toolchain file using the environment variable
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

# Find curl package
find_package(CURL CONFIG REQUIRED)

# Link to libcurl
target_link_libraries(chatdbg PRIVATE CURL::libcurl)
