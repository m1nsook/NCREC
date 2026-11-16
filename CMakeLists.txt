cmake_minimum_required(VERSION 3.24)
project(NCREC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(ncrec_core STATIC
    src/core/CameraPath.cpp
    src/core/Session.cpp
)
target_include_directories(ncrec_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(ncrec_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(ncrec_core PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(ncrec_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(ncrec_core_tests tests/core_tests.cpp)
    target_link_libraries(ncrec_core_tests PRIVATE ncrec_core)
    add_test(NAME ncrec_core_tests COMMAND ncrec_core_tests)
endif()

option(NCREC_BUILD_PLUGIN "Build the Windows ASI bootstrap and independent editor" ${WIN32})
if(NCREC_BUILD_PLUGIN)
    if(NOT WIN32)
        message(FATAL_ERROR "The ASI plugin requires Windows.")
    endif()

    include(FetchContent)
    # The upstream git revision is pinned rather than tracking a moving branch.
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(ncrec_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    target_include_directories(ncrec_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(ncrec_imgui PUBLIC d3d11 dxgi)

    add_library(NCREC SHARED
        src/platform/Plugin.cpp
        src/platform/OverlayWindow.cpp
    )
    target_link_libraries(NCREC PRIVATE ncrec_core ncrec_imgui user32 gdi32)
    target_compile_definitions(NCREC PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    set_target_properties(NCREC PROPERTIES
        PREFIX ""
        SUFFIX ".asi"
        OUTPUT_NAME "NCREC"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/x64/plugins"
    )
    if(MSVC)
        target_compile_options(NCREC PRIVATE /W4 /permissive- /utf-8)
    endif()
endif()
