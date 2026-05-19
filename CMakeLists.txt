cmake_minimum_required(VERSION 4.1)
project(chip8 C)

set(CMAKE_C_STANDARD 23)

# Find SDL2
find_package(SDL2 REQUIRED)

add_custom_target(copy_roms ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/rom"
        "$<TARGET_FILE_DIR:chip8>/rom"
        COMMENT "Copying ROMs to build output directory"
)

add_custom_target(copy_sources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/chip8-source"
        "$<TARGET_FILE_DIR:chip8>/chip8-source"
        COMMENT "Copying Chip-8 source files to build output directory"
)

add_executable(chip8
        src/main.c
        src/display.c
        src/cpu.c
        src/cpu.h
        src/display.h
        src/audio.c
        src/audio.h
        src/assembler.c
        src/assembler.h
)

add_dependencies(chip8 copy_roms copy_sources)

target_include_directories(chip8 PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(chip8 PRIVATE ${SDL2_LIBRARIES} m)
