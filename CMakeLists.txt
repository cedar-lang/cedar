# stop if cmake version below 3.5
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(cedar C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


include_directories(${CMAKE_SOURCE_DIR}/include)

include(${CMAKE_SOURCE_DIR}/src/cedar/CMakeLists.txt)

add_executable(cedar-bin src/main.cc)


target_link_libraries(cedar-bin cedar-lib)

set_target_properties(cedar-bin PROPERTIES OUTPUT_NAME cedar)

target_compile_options(cedar-bin PRIVATE -Wall -Wextra)


install(TARGETS cedar-bin DESTINATION bin)
install(TARGETS cedar-lib DESTINATION lib)
