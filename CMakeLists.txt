cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(wgslx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(WGSLX_COMPILE_OPTIONS
    -fno-rtti
    -Wall
    -Wextra
    -Wno-missing-field-initializers
    -Wno-deprecated-declarations
    -Werror
)

add_subdirectory(third_party EXCLUDE_FROM_ALL SYSTEM)
add_subdirectory(writer)
add_subdirectory(minifier)
add_subdirectory(cmd)
