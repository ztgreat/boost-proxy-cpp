cmake_minimum_required(VERSION 3.16)
project(boost-proxy-cpp)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(include)

# yaml-cpp
include_directories(include/yaml-cpp)
include_directories(include/yaml-cpp/contrib)
include_directories(include/yaml-cpp/node)
include_directories(include/yaml-cpp/node/detail)


aux_source_directory(src DIR_SRCS)
aux_source_directory(. DIR_SRCS)
add_executable(${PROJECT_NAME} ${DIR_SRCS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_HOME_DIRECTORY}/libs/libyaml-cpp.a)