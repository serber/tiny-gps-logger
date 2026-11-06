cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME tiny-gps-logger)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    app/app.c
    app/board.c
    app/csv.c
    app/diskio.c
    app/gps.c
    app/logger.c
    app/policy.c
    app/power.c
    app/sd_spi.c
    app/ubx.c
    third_party/fatfs/ff.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    app
    app/vendor
    third_party/fatfs
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:APP_LOG_ON_USB=1>
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Raw binary for dfu-util, and the size report
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
)

# Flash through the STM32 USB bootloader: cmake --build --preset Debug --target flash-dfu
add_custom_target(flash-dfu
    COMMAND dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D ${CMAKE_PROJECT_NAME}.bin
    DEPENDS ${CMAKE_PROJECT_NAME}
)

# Flash through SWD with an ST-Link on J5
add_custom_target(flash-swd
    COMMAND openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program $<TARGET_FILE:${CMAKE_PROJECT_NAME}> verify reset exit"
    DEPENDS ${CMAKE_PROJECT_NAME}
)
