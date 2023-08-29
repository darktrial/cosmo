cmake_minimum_required(VERSION 3.5)

project(cosmo VERSION 0.1 LANGUAGES CXX)
project(cosmo VERSION 0.1 LANGUAGES C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -DNO_GETIFADDRS
    -DNO_STD_LIB
    -D_WIN32
)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/live/BasicUsageEnvironment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/groupsock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/UsageEnvironment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/liveMedia/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/include/libavcodec
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/include/libavformat
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/include/
)
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib/
    ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/lib/
)
file(GLOB LIVE555_SRC
    live/BasicUsageEnvironment/*.cpp
    live/groupsock/*.cpp
    live/groupsock/*.c
    live/UsageEnvironment/*.cpp
    live/liveMedia/*.hh
    live/liveMedia/*.cpp
    live/liveMedia/*.c
)
set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        tsq.hh
        rtspClientHelper.hh
        mainwindow.ui
        rtspClientHelper.cpp
)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(cosmo
        ${LIVE555_SRC}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        cosmo.qrc
        icon.rc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET cosmo APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(cosmo SHARED
            ${LIVE555_SRC}
            ${PROJECT_SOURCES}
            cosmo.qrc
            icon.rc
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(cosmo
            ${LIVE555_SRC}
            ${PROJECT_SOURCES}
            cosmo.qrc
            icon.rc
        )
    endif()
endif()

target_link_libraries(cosmo PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    ssl
    crypto
    ws2_32
    avformat
    avcodec
    avutil
    bcrypt
)

set_target_properties(cosmo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS cosmo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(cosmo)
endif()
