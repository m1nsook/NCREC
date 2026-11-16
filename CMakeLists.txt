cmake_minimum_required(VERSION 3.24)
project(NCREC VERSION 0.1.0 LANGUAGES C CXX)

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

option(NCREC_BUILD_PLUGIN "Build experimental DX12 in-game editor ASI" ${WIN32})
if(NCREC_BUILD_PLUGIN)
    if(NOT WIN32)
        message(FATAL_ERROR "The ASI plugin requires Windows.")
    endif()

    include(FetchContent)
    # Explicit immutable dependency revisions; no moving main/master tag.
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06
    )
    FetchContent_Declare(minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG 1e9ad1eb42db11bfcb65461f687c656612d1b555
    )
    FetchContent_MakeAvailable(imgui minhook)

    add_library(ncrec_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    )
    target_include_directories(ncrec_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(ncrec_imgui PUBLIC d3d12 dxgi d3dcompiler)

    # MIT-licensed DXGI hook implementation vendored with attribution.
    # Experimental: requires testing with CET/ReShade and Cyberpunk patches.
    add_library(ncrec_dx12_overlay STATIC
        src/platform/dx12/overlay.cpp
    )
    target_include_directories(ncrec_dx12_overlay PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/dx12
    )
    target_link_libraries(ncrec_dx12_overlay PUBLIC ncrec_imgui minhook)

    add_library(NCREC SHARED
        src/platform/Plugin.cpp
        src/platform/GameOverlay.cpp
    )
    target_link_libraries(NCREC PRIVATE ncrec_core ncrec_dx12_overlay user32 gdi32)
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
