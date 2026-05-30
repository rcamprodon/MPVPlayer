cmake_minimum_required(VERSION 3.16)
project(MPVPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

add_executable(MPVPlayer
    main.cpp
    mpvplayer.cpp
)

target_include_directories(MPVPlayer PRIVATE ${MPV_INCLUDE_DIRS})
target_compile_options(MPVPlayer PRIVATE ${MPV_CFLAGS_OTHER})
target_link_directories(MPVPlayer PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(MPVPlayer PRIVATE ${MPV_LIBRARIES})