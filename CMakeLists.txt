cmake_minimum_required(VERSION 3.30)
project(doctor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

###############################################################################
#                                 imgui
###############################################################################

add_library(imgui
  vendor/imgui/imgui.cpp
  vendor/imgui/imgui_demo.cpp # for test
  vendor/imgui/imgui_draw.cpp
  vendor/imgui/imgui_tables.cpp
  vendor/imgui/imgui_widgets.cpp
  vendor/imgui/backends/imgui_impl_win32.cpp
  vendor/imgui/backends/imgui_impl_dx12.cpp
)
target_include_directories(imgui
  PUBLIC
    vendor/imgui
    vendor/imgui/backends
)

###############################################################################


file(GLOB_RECURSE SRC src/*.cpp)
add_library(libdoctor ${SRC})
target_compile_definitions(libdoctor
  PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
if (MSVC)
  target_compile_options(libdoctor PUBLIC /utf-8)
endif()
target_include_directories(libdoctor
  PUBLIC
    include
)
target_link_libraries(libdoctor
  PRIVATE
    d3d12
    dxgi
)

file(GLOB_RECURSE DOCTOR_SRC test/*.cpp)
add_executable(doctor ${DOCTOR_SRC})
target_link_libraries(doctor
  PRIVATE
    libdoctor
    imgui
)
