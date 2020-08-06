cmake_minimum_required(VERSION 2.8)
project(vsomeip-testing)

set(CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package(vsomeip3 3.1.14 REQUIRED)
find_package(Boost 1.65.1 COMPONENTS system thread log REQUIRED)

include_directories(
    ${VSOMEIP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)
SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
SET(GCC_COVERAGE_LINK_FLAGS    "-lgcov")


option(USE_GCC "If gcc is used AFL-specific code is not used" OFF)

if (USE_GCC)
  add_definitions(-DCOMPILE_WITH_GCC)
endif()

option(COVERAGE "If coverage support needs to be compiled in" OFF)

if (COVERAGE)
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
endif()

add_executable(response response.cpp)
add_executable(request request.cpp)
add_executable(publish publish.cpp)
add_executable(fuzzing fuzzing.cpp)
target_link_libraries(response vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(request vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(publish vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(fuzzing vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
