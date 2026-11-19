cmake_minimum_required(VERSION 3.19)

project(ac_energy_saver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# 强制指定使用 MinGW 版本的 Qt 库（适配你当前的 MinGW 编译套件）
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Network
    SerialBus
    Charts
    QuickWidgets
    Qml

)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src/communication)
add_subdirectory(src/logic)
add_subdirectory(src/data)
add_subdirectory(src/ui)
add_subdirectory(src/app)
add_subdirectory(src/simulator)
