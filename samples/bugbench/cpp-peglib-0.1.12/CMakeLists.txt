cmake_minimum_required(VERSION 3.1.0)
project("cpp-peglib")

# Check if a supported compiler is used to setup the C++ standard to use:
get_property(known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
list(FIND known_features "cxx_std_17" found)
if(NOT ${found} EQUAL -1)
  # C++17 standard is supported
  set(CMAKE_CXX_STANDARD 17)
else()
  # Check for C++11 standard support
  list(FIND known_features "cxx_std_11" found)
  if(NOT ${found} EQUAL -1)
    # C++11 standard is supported
    set(CMAKE_CXX_STANDARD 11)
  endif()
endif()

if(${found} EQUAL -1)
  message(FATAL_ERROR "Your compiler is not supported.")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-padded -Wno-weak-vtables -Wno-exit-time-destructors -Wno-c++2a-compat -Wno-switch-enum -Wno-c++98-compat-pedantic")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Woverloaded-virtual")
elseif(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4503 /wd4512 /utf-8")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(add_link_deps Threads::Threads)
endif()

enable_testing()

add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(lint)

install(FILES peglib.h DESTINATION include)
