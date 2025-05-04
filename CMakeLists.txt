cmake_minimum_required(VERSION 3.16)
project(DocumentScanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OpenCV_DIR C:/opencv/opencv/build)
find_package(OpenCV REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.hpp")
#add_executable(doc_ src/scanner.cpp include/scanner.hpp)
#target_link_libraries(doc_ ${OpenCV_LIBS})
add_executable(DocumentScanner ${SOURCES} ${HEADERS} ui/mainwindow.ui)

target_link_libraries(DocumentScanner 
    Qt5::Core
    Qt5::Widgets 
    ${OpenCV_LIBS}
)

#target_include_directories(doc_ PRIVATE include)