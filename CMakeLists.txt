cmake_minimum_required(VERSION 2.8)
project(vsomeip-fuzzing)

set(CMAKE_CXX_FLAGS "-g -std=c++0x")

option(USE_GCC "Set gcc compiler support for AFL coverage report generation" OFF)
option(COVERAGE "Set gcc compiler flags for AFL coverage report generation" OFF)

if (USE_GCC)
  message("-- Enable gcc compiler support for AFL coverage reports")
  add_definitions(-DCOMPILE_WITH_GCC)
endif()

if (COVERAGE)
  message("-- Set gcc compiler flags for AFL coverage reports")
  set(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
  set(GCC_COVERAGE_LINK_FLAGS "-lgcov")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
endif()

find_package(vsomeip3 3.1.14 REQUIRED)
find_package(Boost 1.65.1 COMPONENTS system thread log REQUIRED)

include_directories(
  ${VSOMEIP_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
  /src/vsomeip/implementation
)

add_executable(
  fuzzing
  fuzzing.cpp
  /src/vsomeip/implementation/configuration/src/configuration_impl.cpp
  /src/vsomeip/implementation/plugin/src/plugin_manager_impl.cpp
)

target_link_libraries(fuzzing vsomeip3 vsomeip3-sd vsomeip3-cfg dl ${Boost_LIBRARIES})
