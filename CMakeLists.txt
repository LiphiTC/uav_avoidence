cmake_minimum_required(VERSION 3.16)
project(ew_route_3d VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(route_model
    point3d.h zone.h zone.cpp drone.h drone.cpp environment.h environment.cpp)
target_include_directories(route_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(EW_BUILD_GUI "Build the Qt 3D application" ON)
option(EW_BUILD_TESTS "Build route and movement tests" OFF)

if(EW_BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DExtras 3DInput Multimedia)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DExtras 3DInput Multimedia)

    set(EFFECT_SOURCES
        explosioneffect.h explosioneffect.cpp videotexture.h videotexture.cpp
        dronevisual.h dronevisual.cpp flightaudio.h flightaudio.cpp assets.qrc)

    add_executable(ew_route_3d WIN32 MACOSX_BUNDLE
        main.cpp mainwindow.h mainwindow.cpp mainwindow.ui scene3d.h scene3d.cpp ${EFFECT_SOURCES})
    target_link_libraries(ew_route_3d PRIVATE route_model
        Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::3DCore
        Qt${QT_VERSION_MAJOR}::3DRender Qt${QT_VERSION_MAJOR}::3DExtras
        Qt${QT_VERSION_MAJOR}::3DInput Qt${QT_VERSION_MAJOR}::Multimedia)
    if(MSVC)
        target_compile_options(ew_route_3d PRIVATE /utf-8)
    endif()
endif()

if(EW_BUILD_TESTS)
    enable_testing()
    add_executable(route_tests tests/route_tests.cpp)
    set_target_properties(route_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF)
    target_link_libraries(route_tests PRIVATE route_model)
    add_test(NAME route_tests COMMAND route_tests)
    if(EW_BUILD_GUI)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
        add_executable(window_tests tests/window_tests.cpp
            mainwindow.h mainwindow.cpp mainwindow.ui scene3d.h scene3d.cpp ${EFFECT_SOURCES})
        target_link_libraries(window_tests PRIVATE route_model Qt${QT_VERSION_MAJOR}::Test
            Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::3DCore
            Qt${QT_VERSION_MAJOR}::3DRender Qt${QT_VERSION_MAJOR}::3DExtras
            Qt${QT_VERSION_MAJOR}::3DInput Qt${QT_VERSION_MAJOR}::Multimedia)
        if(MSVC)
            target_compile_options(window_tests PRIVATE /utf-8)
        endif()
        add_test(NAME window_tests COMMAND window_tests)
        set_tests_properties(window_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()
