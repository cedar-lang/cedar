# stop if cmake version below 3.5
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(cedar C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILD_MODE=Debug -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DBUILD_MODE=Release")

set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DBUILD_MODE=Debug -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g -DBUILD_MODE=Release")


include_directories(dep/bdwgc/include)
add_definitions(-DPOINTER_MASK=0x0000FFFFFFFFFFFF)

# build dependencies before setting up project deets
add_subdirectory(dep/libuv)
add_subdirectory(dep/replxx)
# add_subdirectory(dep/bdwgc EXCLUDE_FROM_ALL)


link_directories(/usr/local/lib)
include_directories(/usr/local/include)
include_directories(dep/bdwgc/include)
include_directories(dep/libuv/include)


set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

add_definitions(-DBUILD_DIR="${BUILD_DIR}")

message(STATUS "Build Mode: ${CMAKE_BUILD_TYPE}")

add_compile_options(-frtti)

include_directories(${CMAKE_SOURCE_DIR}/include)

include(${CMAKE_SOURCE_DIR}/src/cedar/CMakeLists.txt)
# target_compile_options(cedar-lib PRIVATE -g -Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing)
# target_compile_options(cedar-lib_a PRIVATE -g -Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing)

# add_executable(cedar-bin src/main.cpp)

# target_link_libraries(cedar-bin cedar-lib_a uv_a ${CMAKE_DL_LIBS} -pthread)
# set_target_properties(cedar-bin PROPERTIES OUTPUT_NAME cedar)

target_compile_options(cedar PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing)


install(TARGETS cedar DESTINATION bin CONFIGURATIONS Release)
install(TARGETS cedar-lib DESTINATION lib CONFIGURATIONS Release)
