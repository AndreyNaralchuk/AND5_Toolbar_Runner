cmake_minimum_required(VERSION 3.21)
project(AND5_Toolbar_Runner C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# The exe goes to bin\ for every configuration.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

add_executable(AND5_Toolbar_Runner WIN32
    src/AND5_main.c
    src/AND5_config.c
    src/AND5_xml.c
    src/AND5_icon.c
    src/AND5_menu.c
    src/AND5_tray.c
    src/AND5_exec.c
    src/AND5_app.h
    src/AND5_resource.h
    src/AND5_app.rc
    src/AND5_app.manifest
)

target_compile_definitions(AND5_Toolbar_Runner PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_compile_options(AND5_Toolbar_Runner PRIVATE /W4)
target_link_libraries(AND5_Toolbar_Runner PRIVATE shell32 shlwapi pathcch xmllite ole32 uxtheme)
