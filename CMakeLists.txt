cmake_minimum_required(VERSION 2.8)
project(vsomeip-testing)

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

set(CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package(vsomeip3 3.1.14 REQUIRED)
find_package(Boost 1.65.1 COMPONENTS system thread log REQUIRED)

include_directories(
  ${VSOMEIP_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
)

add_executable(response response.cpp)
add_executable(request request.cpp)
add_executable(publish publish.cpp)
add_executable(fuzzing fuzzing.cpp)
target_link_libraries(response vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(request vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(publish vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(fuzzing vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
