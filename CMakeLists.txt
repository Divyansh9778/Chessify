cmake_minimum_required(VERSION 3.16)
project(Chessify)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

set(SRC_FILES
    src/main.cpp

    src/Board.cpp
    src/Move.cpp
    src/Piece.cpp
    
    src/Settings.cpp

    src/MovePanel.cpp
    src/MoveHistory.cpp

    src/game/GameController.cpp

    src/network/NetworkClient.cpp
    src/network/NetworkMessages.cpp

    src/network/ProtocolUtils.cpp
)

if (WIN32)
    list(APPEND SRC_FILES src/Engine_win.cpp)
else()
    list(APPEND SRC_FILES src/Engine_unix.cpp)
endif()

add_executable(Chessify ${SRC_FILES} "src/network/PlayerColor.h" "src/network/NetworkProtocol.h" "src/network/NetworkPacket.h" "src/network/ProtocolUtils.h" "src/network/ProtocolUtils.cpp" "server/RoomManager.h" "server/RoomManager.cpp" "server/Room.h")

add_executable(ChessifyServer
    server/server.cpp
 "src/network/PlayerColor.h" "src/network/NetworkProtocol.h" "src/network/NetworkPacket.h" "src/network/ProtocolUtils.h" "src/network/ProtocolUtils.cpp" "server/RoomManager.h" "server/RoomManager.cpp" "server/Room.h")

target_include_directories(Chessify PRIVATE
    src
)

target_compile_definitions(Chessify PRIVATE
    _WIN32_WINNT=0x0A00
)

target_link_libraries(Chessify PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    Boost::headers
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(ChessifyServer PRIVATE
    Boost::system
)

file(COPY ${CMAKE_SOURCE_DIR}/assets
     DESTINATION ${CMAKE_BINARY_DIR})

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL")
endif()