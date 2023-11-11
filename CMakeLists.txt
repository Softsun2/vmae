cmake_minimum_required(VERSION 3.16)

project(vmae VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_executable(vmae
  src/vmae.cpp
)

target_link_libraries(vmae PRIVATE
  PkgConfig::LIBAV
)

install(TARGETS vmae DESTINATION .)
