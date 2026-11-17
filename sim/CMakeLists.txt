cmake_minimum_required(VERSION 3.16)
project(nerdqaxe_display_sim C CXX)

# Desktop simulator for the miner's LVGL screens. It compiles the firmware's
# own UI sources (ui.cpp, themes, fonts) against the same LVGL release the
# firmware pins in main/idf_component.yml, with small stand-ins for the
# ESP-IDF and board headers the UI pulls in.

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FW_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/../main)
set(FW_COMPONENTS ${CMAKE_CURRENT_SOURCE_DIR}/../components)

include(FetchContent)
FetchContent_Declare(lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG v8.3.11
    GIT_SHALLOW TRUE)
FetchContent_GetProperties(lvgl)
if(NOT lvgl_POPULATED)
    cmake_policy(PUSH)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD) # populate only: LVGL's own CMake project is not used
    endif()
    FetchContent_Populate(lvgl)
    cmake_policy(POP)
endif()

file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${lvgl_SOURCE_DIR}/src/*.c)
add_library(lvgl_sim STATIC ${LVGL_SOURCES})
target_include_directories(lvgl_sim PUBLIC ${lvgl_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
# An explicit path: main/displays also holds an lv_conf.h (unused by the
# firmware build, which configures LVGL through Kconfig) that must not win.
target_compile_definitions(lvgl_sim PUBLIC LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h LV_LVGL_H_INCLUDE_SIMPLE=1)
target_compile_options(lvgl_sim PRIVATE -w)

# ui.cpp reaches the board and display driver through relative includes
# ("../boards/board.h", "displayDriver.h"), which resolve next to the source
# file. Stage the UI sources beside stand-in versions of those two headers so
# the real ui.cpp compiles unchanged. configure_file re-runs on every change.
set(STAGE ${CMAKE_BINARY_DIR}/stage)
foreach(f displays/ui.cpp displays/ui.h displays/ui_helpers.cpp displays/ui_helpers.h
          otp/qrcodegen.h macros.h)
    configure_file(${FW_MAIN}/${f} ${STAGE}/${f} COPYONLY)
endforeach()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stubs/board.h ${STAGE}/boards/board.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/stubs/displayDriver.h ${STAGE}/displays/displayDriver.h COPYONLY)

file(GLOB FW_FONTS_AND_ICONS CONFIGURE_DEPENDS ${FW_MAIN}/displays/images/*.c)
file(GLOB FW_QBIT_SOURCES CONFIGURE_DEPENDS ${FW_MAIN}/qbit/*.cpp)

add_executable(nerdqaxe-sim
    src/sim_main.cpp
    src/sim_fill.cpp
    src/sim_scenario.cpp
    src/png_writer.cpp
    src/esp_stubs.cpp
    ${STAGE}/displays/ui.cpp
    ${STAGE}/displays/ui_helpers.cpp
    ${FW_MAIN}/otp/qrcodegen.cpp
    ${FW_MAIN}/displays/images/themes/themes.c
    ${FW_FONTS_AND_ICONS}
    ${FW_QBIT_SOURCES})

target_include_directories(nerdqaxe-sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${STAGE}/displays
    ${STAGE}
    ${FW_MAIN}/displays/images/themes
    ${FW_MAIN}/displays
    ${FW_MAIN}
    ${FW_COMPONENTS}/arduinojson)
target_compile_definitions(nerdqaxe-sim PRIVATE NERDQAXE_SIM=1)
target_link_libraries(nerdqaxe-sim PRIVATE lvgl_sim)
set_source_files_properties(${FW_FONTS_AND_ICONS} ${FW_MAIN}/displays/images/themes/themes.c
    PROPERTIES COMPILE_OPTIONS "-w")

find_package(ZLIB REQUIRED)
target_link_libraries(nerdqaxe-sim PRIVATE ZLIB::ZLIB)

find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
    target_compile_definitions(nerdqaxe-sim PRIVATE SIM_HAVE_SDL=1)
    target_link_libraries(nerdqaxe-sim PRIVATE SDL2::SDL2)
else()
    message(STATUS "SDL2 not found: building the headless PNG renderer only")
endif()
