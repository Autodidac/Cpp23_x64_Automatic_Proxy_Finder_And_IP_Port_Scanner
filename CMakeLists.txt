cmake_minimum_required(VERSION 3.28)
project(InternetScanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(InternetScanner
    src/main.cpp
)

target_sources(InternetScanner
    PRIVATE
        FILE_SET CXX_MODULES
        FILES
            src/httpme.ixx
            src/urlme.ixx
            src/scanner.ixx
            src/version.ixx
        src/gui.ixx
        src/ports.ixx

)

target_link_libraries(InternetScanner PRIVATE ws2_32)

if (MSVC)
    target_compile_options(InternetScanner PRIVATE
        /std:c++latest
        /permissive-
        /Zc:preprocessor
    )
    target_link_options(InternetScanner PRIVATE /SUBSYSTEM:WINDOWS)
endif()
