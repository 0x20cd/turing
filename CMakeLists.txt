cmake_minimum_required(VERSION 3.5)

project(app-turing VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp res.qrc
    src/tur/emulator.hpp src/tur/emulator.cpp
    src/tur/loader.hpp src/tur/loader.cpp
    src/tur/tokenizer.hpp src/tur/tokenizer.cpp
    src/tur/common.hpp
    src/ui/mainwindow.ui src/ui/mainwindow.h src/ui/mainwindow.cpp
    src/ui/cell.h src/ui/cell.cpp
)

qt_add_executable(app-turing
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
qt_add_translations(app-turing TS_FILES turing_ru.ts)

target_link_libraries(app-turing PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(app-turing PRIVATE
    src
)

set_target_properties(app-turing PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS app-turing
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(app-turing)
endif()
