cmake_minimum_required(VERSION 3.31)
project(web C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED IMPORTED_TARGET libmicrohttpd)

add_executable(web main.c)

target_compile_options(web PRIVATE -Wall)

target_link_libraries(web PRIVATE PkgConfig::MICROHTTPD)