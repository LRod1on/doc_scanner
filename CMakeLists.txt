cmake_minimum_required(VERSION 3.10)
project(DocumentScanner)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----  GLFW

set(GLFW_ROOT "C:/Users/ASUs/doc_scanner/thirdparty/glfw")

include_directories("${GLFW_ROOT}/include")
link_directories("${GLFW_ROOT}/lib")

# ---- ImGui
set(IMGUI_ROOT "C:/Users/ASUs/doc_scanner/thirdparty/imgui")

include_directories("${IMGUI_ROOT}")  
include_directories(
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"  
)

file(GLOB IMGUI_SOURCES
    "${IMGUI_ROOT}/*.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp"
)   

# ---- Настройка OpenCV
set(OpenCV_DIR C:/opencv/opencv/build)
find_package(OpenCV REQUIRED)

# ---- Настройка DocumentScanner.exe

add_executable(DocumentScanner
    src/main_window_copy.cpp
    src/scanner.cpp
    ${IMGUI_SOURCES}  
)

target_include_directories(DocumentScanner PRIVATE
    include/
    ${GLFW_ROOT}/include
    ${IMGUI_ROOT}
    ${IMGUI_ROOT}/backends
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(DocumentScanner
    ${OpenCV_LIBS}
    ${GLFW_ROOT}/lib/glfw3.lib
    opengl32.lib  
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:CONSOLE")
endif()