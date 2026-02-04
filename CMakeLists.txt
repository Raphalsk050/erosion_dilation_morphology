cmake_minimum_required(VERSION 3.16)
project(erosion_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find SDL2
find_package(SDL2 REQUIRED)

# Find OpenGL (required for ImGui SDL2+OpenGL3 backend)
find_package(OpenGL REQUIRED)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Common source files
set(COMMON_SOURCES
    src/binary_image.cpp
)

# Include directories (common)
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries (common)
set(COMMON_LIBS
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# ========================================
# Target 1: Morphology Demo (erosion_demo)
# ========================================
set(EROSION_SOURCES
    src/main.cpp
    src/erosion.cpp
    src/visualizer.cpp
    ${COMMON_SOURCES}
    ${IMGUI_SOURCES}
)

add_executable(erosion_demo ${EROSION_SOURCES})
target_include_directories(erosion_demo PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(erosion_demo PRIVATE ${COMMON_LIBS})

# ========================================
# Target 2: Flood Fill Demo (floodfill_demo)
# ========================================
set(FLOODFILL_SOURCES
    src/main_floodfill.cpp
    src/floodfill.cpp
    src/floodfill_visualizer.cpp
    ${COMMON_SOURCES}
    ${IMGUI_SOURCES}
)

add_executable(floodfill_demo ${FLOODFILL_SOURCES})
target_include_directories(floodfill_demo PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_libraries(floodfill_demo PRIVATE ${COMMON_LIBS})

# ========================================
# macOS specific settings
# ========================================
if(APPLE)
    # Try to find SDL2 via pkg-config first (homebrew)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PKG QUIET sdl2)
        if(SDL2_PKG_FOUND)
            target_include_directories(erosion_demo PRIVATE ${SDL2_PKG_INCLUDE_DIRS})
            target_link_libraries(erosion_demo PRIVATE ${SDL2_PKG_LIBRARIES})
            target_include_directories(floodfill_demo PRIVATE ${SDL2_PKG_INCLUDE_DIRS})
            target_link_libraries(floodfill_demo PRIVATE ${SDL2_PKG_LIBRARIES})
        endif()
    endif()
    
    # OpenGL framework on macOS
    target_link_libraries(erosion_demo PRIVATE "-framework OpenGL")
    target_link_libraries(floodfill_demo PRIVATE "-framework OpenGL")
endif()
