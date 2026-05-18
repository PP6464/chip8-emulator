cmake_minimum_required(VERSION 4.1)
project(chip8 C)

set(CMAKE_C_STANDARD 23)

# Find SDL2
find_package(SDL2 REQUIRED)

# 1. Create a custom target that always copies the directory
add_custom_target(copy_roms ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/rom"
        "$<TARGET_FILE_DIR:chip8>/rom"
        COMMENT "Copying ROMs to build output directory"
)

# 2. Define the main executable
add_executable(chip8
        src/main.c
        src/display.c
        src/cpu.c
        src/cpu.h
        src/display.h
        src/audio.c
        src/audio.h
)

# 3. Force chip8 to wait for copy_roms to finish first
add_dependencies(chip8 copy_roms)

target_include_directories(chip8 PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(chip8 PRIVATE ${SDL2_LIBRARIES} m)
