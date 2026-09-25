cmake_minimum_required(VERSION 3.16)
project(KrythorixChess CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Help CMake find Homebrew-installed raylib on both Apple Silicon
# (/opt/homebrew) and Intel Macs (/usr/local).
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)
endif()

find_package(raylib REQUIRED)

add_executable(ChessGame
    Source.cpp
    Board.cpp
    Piece.cpp
    Pawn.cpp
    Rook.cpp
    Knight.cpp
    Bishop.cpp
    Queen.cpp
    King.cpp
    utility.cpp
    UCIEngine.cpp
)

target_link_libraries(ChessGame raylib)

# macOS needs these system frameworks for raylib to link
if(APPLE)
    target_link_libraries(ChessGame
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

# Locate open-source Stockfish engine if installed
find_program(STOCKFISH_EXE NAMES stockfish PATHS /opt/homebrew/bin /usr/local/bin)

# Copy the PNGs and sounds asset folders next to the built executable so
# LoadTexture("PNGs/....png") and LoadSound("sounds/....wav") resolve
# correctly at runtime.
add_custom_command(TARGET ChessGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/PNGs
        $<TARGET_FILE_DIR:ChessGame>/PNGs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sounds
        $<TARGET_FILE_DIR:ChessGame>/sounds
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/haha.txt
        $<TARGET_FILE_DIR:ChessGame>/haha.txt
)

if(STOCKFISH_EXE)
    add_custom_command(TARGET ChessGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${STOCKFISH_EXE}
            $<TARGET_FILE_DIR:ChessGame>/stockfish
    )
endif()
