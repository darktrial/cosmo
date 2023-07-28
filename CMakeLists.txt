cmake_minimum_required(VERSION 3.14)

project(live555test LANGUAGES CXX)
project(live555test LANGUAGES C)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -DNO_GETIFADDRS
    -DNO_STD_LIB
    -D_WIN32
)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/live/BasicUsageEnvironment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/groupsock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/UsageEnvironment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live/liveMedia/include
)
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl/lib
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

add_executable(live555test
    ${LIVE555_SRC}
  main.cpp
)
target_link_libraries(live555test
    Qt${QT_VERSION_MAJOR}::Core
    ssl
    crypto
    ws2_32
)

install(TARGETS live555test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
