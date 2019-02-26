# stop if cmake version below 3.5
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(cedar C CXX)

# build dependencies before setting up project deets
add_subdirectory(${CMAKE_SOURCE_DIR}/dep/libuv)
# add_subdirectory(${CMAKE_SOURCE_DIR}/dep/bdwgc)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


link_directories(/usr/local/lib)
include_directories(/usr/local/include)
include_directories(dep/bdwgc/include)
include_directories(dep/libuv/include)



set (CMAKE_CXX_STANDARD 17)




set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILD_MODE=Debug")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DBUILD_MODE=Release")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")


add_definitions(-DCORE_DIR="${CORE_DIR}")


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
