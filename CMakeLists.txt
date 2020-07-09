cmake_minimum_required (VERSION 2.8)

set (CMAKE_CXX_FLAGS "-g -std=c++0x")

find_package (vsomeip3 3.1.14 REQUIRED)
find_package (Boost 1.65.1 COMPONENTS system thread log REQUIRED)

include_directories (
    ${VSOMEIP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)

add_executable(service service.cpp)
add_executable(client client.cpp)
target_link_libraries(service vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
target_link_libraries(client vsomeip3 vsomeip3-sd ${Boost_LIBRARIES})
