cmake_minimum_required(VERSION 3.16)

project(tm-emu VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick)

qt_standard_project_setup()

qt_add_executable(apptm-emu
    src/main.cpp
    src/tur/emulator.hpp src/tur/emulator.cpp
)

qt_add_resources(apptm-emu "apptm-emu"
    PREFIX "/"
    FILES "src/qml/Main.qml"
)

set_target_properties(apptm-emu PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(apptm-emu PUBLIC
    src
)

target_link_libraries(apptm-emu
    PRIVATE Qt6::Quick
)

install(TARGETS apptm-emu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
