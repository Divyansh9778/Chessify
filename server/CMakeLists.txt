cmake_minimum_required(VERSION 3.16)

project(ChessifyServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)

add_executable(ChessifyServer
    server.cpp
    ../src/network/ProtocolUtils.cpp
)

target_include_directories(ChessifyServer PRIVATE
    ../src
    ../src/network
)

target_link_libraries(ChessifyServer PRIVATE
    Boost::boost
)