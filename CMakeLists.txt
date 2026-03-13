cmake_minimum_required(VERSION 3.16)
project(Chessify)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

set(SRC_FILES
    src/main.cpp
    src/Board.cpp
    src/Move.cpp
    src/Piece.cpp
    src/Settings.cpp
    src/MovePanel.cpp
    src/MoveHistory.cpp
    src/Engine_unix.cpp
    src/Engine_win.cpp
)

if (WIN32)
    list(APPEND SRC_FILES src/Engine_win.cpp)
else()
    list(APPEND SRC_FILES src/Engine_unix.cpp)
endif()

add_executable(Chessify ${SRC_FILES})

target_include_directories(Chessify PRIVATE src)

target_link_libraries(Chessify PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)

file(COPY ${CMAKE_SOURCE_DIR}/assets
     DESTINATION ${CMAKE_BINARY_DIR})

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL")
endif()