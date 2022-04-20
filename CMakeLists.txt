cmake_minimum_required(VERSION 2.8)
project(vsomeip-fuzzing)

set(CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package(vsomeip3 3.1.14 REQUIRED)
find_package(Boost 1.71 COMPONENTS system thread log REQUIRED)

include_directories(
  ${VSOMEIP_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
)

add_executable(response response.cpp)
add_executable(request request.cpp)

target_link_libraries(response vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(request vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
