cmake_minimum_required(VERSION 3.10)
project(SAT_DPLL)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую генерацию moc-файлов (для Qt-классов, если вдруг понадобятся)
set(CMAKE_AUTOMOC ON)

# Находим пакет Qt5 (модуль Core)
find_package(Qt5 COMPONENTS Core REQUIRED)

add_executable(sat_dpll
    main.cpp
    BBV.cpp
    boolinterval.cpp
    boolequation.cpp
    MinOccStrategy.cpp

    ${CMAKE_SOURCE_DIR}/allocator/Allocator.cpp
)

# Подключаем нужные include-пути и линкуем QtCore
target_link_libraries(sat_dpll
    PRIVATE
        Qt5::Core
)

target_include_directories(sat_dpll PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/allocator)
