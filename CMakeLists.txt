cmake_minimum_required(VERSION 3.21)

project(DownloadX VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(DownloadX
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h

    core/DownloadManager.cpp
    core/DownloadManager.h
)

target_link_libraries(DownloadX PRIVATE
    Qt6::Widgets
    Qt6::Network
)
