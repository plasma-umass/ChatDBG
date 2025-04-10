cmake_minimum_required(VERSION 2.8)
project(example)

include_directories(..)

if(MSVC)
  add_compile_options(${cxx11_options} /W3)
else()
  add_compile_options(${cxx11_options})
endif()

add_executable(calc calc.cc)
target_link_libraries(calc ${add_link_deps})

add_executable(calc2 calc2.cc)
target_link_libraries(calc2 ${add_link_deps})

add_executable(calc3 calc3.cc)
target_link_libraries(calc3 ${add_link_deps})

add_executable(calc4 calc4.cc)
target_link_libraries(calc4 ${add_link_deps})

add_executable(calc5 calc5.cc)
target_link_libraries(calc5 ${add_link_deps})
